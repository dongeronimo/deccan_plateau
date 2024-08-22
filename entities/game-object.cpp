#include "game-object.h"
#include <stdexcept>
#include "object_namer.h"
#include "concatenate.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "my-vk.h"
static VkDescriptorPool gObjectDescriptorPool = VK_NULL_HANDLE;
namespace entities {
    GameObject::GameObject(VkContext* ctx, const std::string& name, VkBuffer vertexBuffer, VkBuffer indexBuffer, uint16_t numberOfIndices):
        mName(name), mDevice(ctx->device),mVertexBuffer(vertexBuffer),
        mIndexBuffer(indexBuffer),mNumberOfIndices(numberOfIndices),
        mOrientation(glm::quat()), mPosition(glm::vec3(0,0,0))
    {
        //initialize the object descriptor pool if is not initialized
        if (gObjectDescriptorPool == VK_NULL_HANDLE) {
            //object descriptor pool - there can be up to 100 objects 
            VkDescriptorPoolSize objectPoolSize{};
            objectPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            objectPoolSize.descriptorCount = MAX_NUMBER_OF_GAME_OBJECTS * MAX_FRAMES_IN_FLIGHT; // Example: support up to 100 objects

            VkDescriptorPoolCreateInfo objectPoolInfo{};
            objectPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            objectPoolInfo.poolSizeCount = 1;
            objectPoolInfo.pPoolSizes = &objectPoolSize;
            objectPoolInfo.maxSets = MAX_NUMBER_OF_GAME_OBJECTS * MAX_FRAMES_IN_FLIGHT; // Support up to 100 descriptor sets for objects
            if (vkCreateDescriptorPool(ctx->device, &objectPoolInfo, nullptr, &gObjectDescriptorPool) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create object descriptor pool!");
            }
            SET_NAME(gObjectDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "objectDescriptorPool");
        }
        assert(gObjectDescriptorPool != VK_NULL_HANDLE);
        //create the uniform buffer
        VkDeviceSize bufferSize = sizeof(ObjectUniformBuffer);
        helloObjectUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        helloObjectUniformBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
        helloObjectUniformBufferAddress.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            CreateBuffer(bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, //will be used as uniform buffer
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //visible to both gpu and cpu
                helloObjectUniformBuffer[i], helloObjectUniformBufferMemory[i], *ctx);
            //map the memory, forever    
            vkMapMemory(ctx->device, helloObjectUniformBufferMemory[i], 0,
                bufferSize, 0, &helloObjectUniformBufferAddress[i]);
            std::string buffName = Concatenate(mName, "ObjectBuffer", i);
            SET_NAME(helloObjectUniformBuffer[i], VK_OBJECT_TYPE_BUFFER, buffName.c_str());
            std::string memName = Concatenate(mName, "ObjectBufferDeviceMemory", i);
            SET_NAME(helloObjectUniformBufferMemory[i], VK_OBJECT_TYPE_DEVICE_MEMORY, memName.c_str());
        }
        //create the descriptor sets
        //one layout for each frame in flight, create the vector filling with the layout that i alredy have
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,ctx->helloObjectDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = gObjectDescriptorPool;
        //one descriptor set for each frame in flight
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();
        helloObjectDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(mDevice, &allocInfo, helloObjectDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets for camera!");
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = helloObjectUniformBuffer[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(ObjectUniformBuffer);
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = helloObjectDescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            vkUpdateDescriptorSets(mDevice, 1, &descriptorWrite, 0, nullptr);
            const std::string dsName = Concatenate(mName, "DescriptorSet", i);
            SET_NAME(helloObjectDescriptorSets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET, dsName.c_str());
        }
    }
    void GameObject::DestroyDescriptorPool(VkDevice device)
    {
        assert(gObjectDescriptorPool != VK_NULL_HANDLE);
        vkDestroyDescriptorPool(device, gObjectDescriptorPool, nullptr);
    }
    GameObject::~GameObject()
    {
        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(mDevice, helloObjectUniformBuffer[i], nullptr);
            vkUnmapMemory(mDevice, helloObjectUniformBufferMemory[i]);
            vkFreeMemory(mDevice, helloObjectUniformBufferMemory[i], nullptr);
        }
        //std::vector<VkBuffer> helloObjectUniformBuffer;
        //std::vector<VkDeviceMemory> helloObjectUniformBufferMemory;
        std::vector<void*> helloObjectUniformBufferAddress;
        std::vector<VkDescriptorSet> helloObjectDescriptorSets;
    }
    void GameObject::CommitDataToObjectBuffer(uint32_t currentFrame)
    {
        ObjectUniformBuffer objBuffer;
        objBuffer.model = glm::mat4(1.0f);
        objBuffer.model *= glm::translate(glm::mat4(1.0f), mPosition);
        objBuffer.model *= glm::mat4_cast(mOrientation);
        memcpy(helloObjectUniformBufferAddress[currentFrame], &objBuffer, sizeof(objBuffer));
    }
}
