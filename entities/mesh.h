#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "io/mesh-data.h"
struct VkContext;

namespace entities {
    //The vertex data structure, for a 3d vertex and it's color
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
    };
    class Mesh {
    public:
        Mesh(const std::vector<Vertex>& vertexes,
             const std::vector<uint16_t>& indices,
             VkContext* ctx,
             const std::string& name);
        Mesh(io::MeshData& meshData, VkContext* ctx);
        ~Mesh();
        const VkContext* mCtx;
        const std::string mName;
        void Bind(VkCommandBuffer cmd)const;
        uint16_t NumberOfIndices()const {
            return mNumberOfIndices;
        }
    private:
        void CtorStartAssertions();
        void CtorInitGlobalMeshBuffer(VkContext* ctx);
        void CtorCopyDataToGlobalBuffer(
            const std::vector<Vertex>& vertexes,
            const std::vector<uint16_t>& indices,
            VkContext* ctx);
        VkDeviceSize mVertexesOffset;
        VkDeviceSize mIndexesOffset;
        uint16_t mNumberOfIndices;
        
    };
}