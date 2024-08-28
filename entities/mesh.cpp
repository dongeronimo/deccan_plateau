#include "mesh.h"
#include "my-vk.h"
#include <stdexcept>
#include "object_namer.h"
#include "commandBufferUtils.h"
#define _256mb 256 * 1024 * 1024
static VkBuffer gMeshBuffer = VK_NULL_HANDLE;
static VkDeviceMemory gMeshMemory = VK_NULL_HANDLE;
uint32_t meshCounter = 0;
uintptr_t gMemoryCursor = 0;
uint32_t _findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkContext ctx) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}


namespace entities {
    Mesh::Mesh(const std::vector<Vertex>& vertexes, 
        const std::vector<uint16_t>& indices,  VkContext* ctx, const std::string& name)
        :mCtx(ctx), mIndexesOffset(LLONG_MAX), mVertexesOffset(LLONG_MAX), mName(name)
    {
        CtorStartAssertions();
        //If this is the first mesh then we have to create the infrastructure.
        CtorInitGlobalMeshBuffer(ctx);
        assert(gMeshBuffer != nullptr);
        assert(gMeshMemory != nullptr);
        meshCounter++;
        CtorCopyDataToGlobalBuffer(vertexes, indices, ctx);
        
    }
    Mesh::Mesh(io::MeshData& meshData, VkContext* ctx):
        mCtx(ctx), mName(meshData.name)
    {
        CtorStartAssertions();
        //If this is the first mesh then we have to create the infrastructure.
        CtorInitGlobalMeshBuffer(ctx);
        assert(gMeshBuffer != nullptr);
        assert(gMeshMemory != nullptr);
        meshCounter++;
        std::vector<entities::Vertex> vertices(meshData.vertices.size());
        for (auto i = 0; i < meshData.vertices.size(); i++) {
            vertices[i].pos = meshData.vertices[i];
            vertices[i].color = meshData.normals[i];//TODO mesh: for now using normal as color
        }
        std::vector<uint16_t> indices = meshData.indices;
        CtorCopyDataToGlobalBuffer(vertices, indices, ctx);

    }
    Mesh::~Mesh()
    {
        assert(mCtx != nullptr && 
            mCtx->device != VK_NULL_HANDLE && 
            gMeshBuffer != VK_NULL_HANDLE && 
            gMeshMemory != VK_NULL_HANDLE);
        meshCounter--;
        if (meshCounter == 0) {
            vkFreeMemory(mCtx->device, gMeshMemory, nullptr);
            vkDestroyBuffer(mCtx->device, gMeshBuffer, nullptr);
        }
    }
    void Mesh::Bind(VkCommandBuffer cmd) const
    {
        assert(mIndexesOffset != LLONG_MAX);
        assert(mVertexesOffset != LLONG_MAX);
        vkCmdBindVertexBuffers(cmd, 0, 1, &gMeshBuffer, &mVertexesOffset);
        vkCmdBindIndexBuffer(cmd, gMeshBuffer, mIndexesOffset, VK_INDEX_TYPE_UINT16);
    }
    void Mesh::CtorStartAssertions()
    {
        assert(mCtx != nullptr);
        assert(mCtx->device != VK_NULL_HANDLE);
        assert(mCtx->commandPool != VK_NULL_HANDLE);
        assert(mCtx->graphicsQueue != VK_NULL_HANDLE);
    }
    void Mesh::CtorInitGlobalMeshBuffer(VkContext* ctx)
    {
        //If this is the first mesh then we have to create the infrastructure.
        if (gMeshBuffer == VK_NULL_HANDLE && gMeshMemory == VK_NULL_HANDLE) {
            assert(meshCounter == 0);
            VkDeviceSize vbSize = _256mb; //256 mb for meshes
            //Buffer description
            VkBufferCreateInfo vbBufferInfo{};
            vbBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vbBufferInfo.size = vbSize;
            vbBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            vbBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            //create the buffer
            if (vkCreateBuffer(mCtx->device, &vbBufferInfo, nullptr, &gMeshBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }
            //the memory the buffer will require, not necessarely equals to the size of the data being stored
            //in it.
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(mCtx->device, gMeshBuffer, &memRequirements);
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = _findMemoryType(memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *mCtx);
            //Allocate the memory
            if (vkAllocateMemory(mCtx->device, &allocInfo, nullptr, &gMeshMemory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }
            vkBindBufferMemory(mCtx->device, gMeshBuffer, gMeshMemory, 0);
            SET_NAME(gMeshBuffer, VK_OBJECT_TYPE_BUFFER, "Global Mesh Buffer");
            SET_NAME(gMeshMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "Global Mesh Memory");
        }
    }
    void Mesh::CtorCopyDataToGlobalBuffer(const std::vector<Vertex>& vertexes, const std::vector<uint16_t>& indices, VkContext* ctx)
    {
        //copy from the vectors to the gpu using staging buffers. Mind the offsets.
        //1)Create a local buffer for the vertex buffer
        VkDeviceSize vertexBufferSize = sizeof(vertexes[0]) * vertexes.size();
        //creates the staging buffer
        VkBuffer vertexStagingBuffer;
        VkDeviceMemory vertexStagingBufferMemory;
        CreateBuffer(vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //Used as source from memory transfers
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //memory is visibe to the cpu and gpu
            vertexStagingBuffer, vertexStagingBufferMemory, *ctx);
        void* vertexStagingBufferAddress;
        vkMapMemory(ctx->device, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexStagingBufferAddress);
        memcpy(vertexStagingBufferAddress, vertexes.data(), (size_t)vertexBufferSize);
        vkUnmapMemory(ctx->device, vertexStagingBufferMemory);
        //2)Create a local buffer for the index buffer
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer indexStagingBuffer;
        VkDeviceMemory indexStagingBufferMemory;
        CreateBuffer(indexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //Used as source from memory transfers
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //memory is visibe to the cpu and gpu
            indexStagingBuffer, indexStagingBufferMemory, *ctx);
        void* indexStagingBufferAddress;
        vkMapMemory(ctx->device, indexStagingBufferMemory, 0, indexBufferSize, 0, &indexStagingBufferAddress);
        memcpy(indexStagingBufferAddress, indices.data(), (size_t)indexBufferSize);
        vkUnmapMemory(ctx->device, indexStagingBufferMemory);
        assert(gMemoryCursor + vertexBufferSize + indexBufferSize < _256mb); //Is there enough space?
        //3)Copy the vertex and index buffer to the main buffer, increase the cursor
        VkCommandBuffer vbCopyCommandBuffer = CreateCommandBuffer(ctx->commandPool,
            ctx->device, "vertex buffer copy command buffer");
        BeginRecordingCommands(vbCopyCommandBuffer);
        CopyBuffer(0, gMemoryCursor, vertexBufferSize, vbCopyCommandBuffer, vertexStagingBuffer, gMeshBuffer);
        mVertexesOffset = gMemoryCursor;
        gMemoryCursor += vertexBufferSize;
        CopyBuffer(0, gMemoryCursor, indexBufferSize, vbCopyCommandBuffer, indexStagingBuffer, gMeshBuffer);
        mIndexesOffset = gMemoryCursor;
        gMemoryCursor += indexBufferSize;
        //4)Finish execution
        SubmitAndFinishCommands(vbCopyCommandBuffer, ctx->graphicsQueue, ctx->device, ctx->commandPool);
        mNumberOfIndices = indices.size();

        vkDestroyBuffer(ctx->device, vertexStagingBuffer, nullptr);
        vkDestroyBuffer(ctx->device, indexStagingBuffer, nullptr);
        vkFreeMemory(ctx->device, vertexStagingBufferMemory, nullptr);
        vkFreeMemory(ctx->device, indexStagingBufferMemory, nullptr);
    }
}
