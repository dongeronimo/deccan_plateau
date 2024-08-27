#include "game-object.h"
#include <stdexcept>
#include "object_namer.h"
#include "concatenate.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "my-vk.h"
#include <map>
/// <summary>
/// The global pool for object buffer uniforms
/// </summary>
static entities::GameObjectUniformBufferPool* gUniformBufferPool = nullptr;
/// <summary>
/// List of available id for game objects. Its important because there's a limited number of possible
/// game objects, limited by the number of descriptor sets in the pool and memory allocated on the 
/// uniform buffer. The game object id is directly related to the offset in the memory and the specific
/// descriptor set bound to that offset. 
/// </summary>
static std::map<uint32_t, bool> gAvailableGameObjectsIds;
/// <summary>
/// Returns an id in [0, MAX_NUMBER_OF_GAME_OBJECTS). If all IDs are used it blows up with runtime error
/// </summary>
/// <returns></returns>
uint32_t GetNextGameObjectId() {
    //initialize the list
    if (gAvailableGameObjectsIds.size() == 0) {
        for (uint32_t i = 0; i < MAX_NUMBER_OF_GAME_OBJECTS; i++) {
            gAvailableGameObjectsIds.insert({ i, true });
        }
    }
    int key = UINT32_MAX;
    for (const auto& [k, v] : gAvailableGameObjectsIds) {
        if (v == true) {
            key = k;
            break;
        }
    }
    if (key == UINT32_MAX) {
        throw std::runtime_error("all possible game objects are alredy allocated");
    }
    return key;
}

namespace entities {
    GameObject::GameObject(VkContext* ctx, const std::string& name, const Mesh* mesh):
        mName(name), mDevice(ctx->device),mMesh(mesh),
        mOrientation(glm::quat()), mPosition(glm::vec3(0,0,0)),
        mId(GetNextGameObjectId())
    {
        assert(mesh != nullptr);
        assert(gAvailableGameObjectsIds.size() == MAX_NUMBER_OF_GAME_OBJECTS);
        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto objects = GameObjectUniformBufferPool::Get(mId*MAX_FRAMES_IN_FLIGHT + i); //the vk objects are contiguous.
            uintptr_t offset = objects.first;
            VkDescriptorSet descriptorSet = objects.second;
            helloObjectUniformBufferAddress[i] = offset;
            helloObjectDescriptorSets[i] = descriptorSet;
        }
        //i'm using this game object slot
        gAvailableGameObjectsIds[mId] = false;
    }
    GameObject::~GameObject()
    {
        //give the slot back
        gAvailableGameObjectsIds[mId] = true;
    }
    void GameObject::CommitDataToObjectBuffer(uint32_t currentFrame)
    {
        ObjectUniformBuffer objBuffer;
        objBuffer.model = glm::mat4(1.0f);
        objBuffer.model *= glm::translate(glm::mat4(1.0f), mPosition);
        objBuffer.model *= glm::mat4_cast(mOrientation);
        void* addr = reinterpret_cast<void*>(GameObjectUniformBufferPool::BaseAddress() + 
            helloObjectUniformBufferAddress[currentFrame]);
        memcpy(addr, &objBuffer, sizeof(objBuffer));
    }

    uint32_t GameObjectUniformBufferPool::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkContext ctx)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    GameObjectUniformBufferPool::GameObjectUniformBufferPool(VkContext* ctx)
        :mCtx(ctx)
    {
        const int number_of_buffers = MAX_NUMBER_OF_GAME_OBJECTS * MAX_FRAMES_IN_FLIGHT;
        //Create the descriptor pool
        //object descriptor pool - there can be up to 100 objects 
        VkDescriptorPoolSize objectPoolSize{};
        objectPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        objectPoolSize.descriptorCount = number_of_buffers; // Example: support up to 100 objects
        VkDescriptorPoolCreateInfo objectPoolInfo{};
        objectPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        objectPoolInfo.poolSizeCount = 1;
        objectPoolInfo.pPoolSizes = &objectPoolSize;
        objectPoolInfo.maxSets = number_of_buffers; // Support up to 100 descriptor sets for objects
        if (vkCreateDescriptorPool(ctx->device, &objectPoolInfo, nullptr, &mObjectDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create object descriptor pool!");
        }
        SET_NAME(mObjectDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "objectDescriptorPool");
        /////allocate the memory
        /////1) create the big buffer
        VkBufferCreateInfo bigAssBufferInfo{};
        bigAssBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bigAssBufferInfo.size = sizeof(ObjectUniformBuffer) * number_of_buffers;
        bigAssBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bigAssBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        mBigBufferForDeviceMemoryAllocation = VK_NULL_HANDLE;
        vkCreateBuffer(ctx->device, &bigAssBufferInfo, nullptr, &mBigBufferForDeviceMemoryAllocation);
        SET_NAME(mBigBufferForDeviceMemoryAllocation, VK_OBJECT_TYPE_BUFFER, "BigAssBufferForObjectUniform");
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(ctx->device, mBigBufferForDeviceMemoryAllocation, &memRequirements);
        VkMemoryAllocateInfo memoryAllocInfo{};
        memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocInfo.allocationSize = memRequirements.size;
        memoryAllocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *ctx);
        //Allocate the memory for the big buffer
        vkAllocateMemory(ctx->device, &memoryAllocInfo, nullptr, &mBuffersMemory);
        vkBindBufferMemory(ctx->device, mBigBufferForDeviceMemoryAllocation, mBuffersMemory, 0);
        //now that i have the buffer and the memory we can create the descriptor sets and update them.
        //one descriptor set for each buffer. 
        assert(ctx->helloObjectDescriptorSetLayout != VK_NULL_HANDLE);
        std::vector<VkDescriptorSetLayout> layouts(number_of_buffers, ctx->helloObjectDescriptorSetLayout);
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool = mObjectDescriptorPool;
        //one descriptor set for each frame in flight
        descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_NUMBER_OF_GAME_OBJECTS);
        descriptorSetAllocInfo.pSetLayouts = layouts.data();
        vkAllocateDescriptorSets(ctx->device, &descriptorSetAllocInfo, mDescriptorSets.data());
        //update each descriptor set. Mind the offset
        for (size_t i = 0; i < number_of_buffers; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mBigBufferForDeviceMemoryAllocation;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(ObjectUniformBuffer);
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mDescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, nullptr);
            mUniformBufferOffsets[i] = sizeof(ObjectUniformBuffer) * i;
        }
        //map the memory
        vkMapMemory(ctx->device, mBuffersMemory, 0,
            sizeof(ObjectUniformBuffer) * number_of_buffers, 0, &mBaseAddress);
        //now we are done - we have the descriptor sets, mapped to positions in the buffer, and memories for these positions.
    }

    GameObjectUniformBufferPool::~GameObjectUniformBufferPool()
    {
        vkUnmapMemory(mCtx->device, mBuffersMemory);
        vkFreeMemory(mCtx->device, mBuffersMemory, nullptr);
        vkDestroyBuffer(mCtx->device, mBigBufferForDeviceMemoryAllocation, nullptr);
        vkDestroyDescriptorPool(mCtx->device, mObjectDescriptorPool, nullptr);
    }

    std::pair<uintptr_t, VkDescriptorSet> GameObjectUniformBufferPool::Get(uint32_t i)
    {
        assert(gUniformBufferPool != nullptr);
        return std::pair<uintptr_t, VkDescriptorSet>(gUniformBufferPool->mUniformBufferOffsets[i],
            gUniformBufferPool->mDescriptorSets[i]);
    }



    void GameObjectUniformBufferPool::Initialize(VkContext* ctx)
    {
        assert(gUniformBufferPool == nullptr);
        gUniformBufferPool = new GameObjectUniformBufferPool(ctx);
    }
    uintptr_t GameObjectUniformBufferPool::BaseAddress()
    {
        assert(gUniformBufferPool != nullptr);
        return reinterpret_cast<uintptr_t>(gUniformBufferPool->mBaseAddress);
    }
    void GameObjectUniformBufferPool::Destroy()
    {
        assert(gUniformBufferPool != nullptr);
        delete gUniformBufferPool;
    }
}
