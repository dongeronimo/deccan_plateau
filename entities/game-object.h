#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/quaternion.hpp>
#define MAX_NUMBER_OF_GAME_OBJECTS 100
struct VkContext;
namespace entities {
    /// <summary>
    /// Holds object-specific data. Corresponds to ObjectUniformBuffer in 
    /// hello_shader.vert
    /// </summary>
    struct alignas(16) ObjectUniformBuffer {
        /// <summary>
        /// Model matrix
        /// </summary>
        alignas(16)glm::mat4 model;
    };
    class GameObject {
    public:
        GameObject(VkContext* ctx, 
            const std::string& name, 
            VkBuffer vertexBuffer,
            VkBuffer indexBuffer,
            uint16_t numberOfIndices);
        void SetPosition(glm::vec3& pos) {
            mPosition = pos;
        }
        glm::vec3 GetPosition()const { return mPosition; }
        void SetOrientation(glm::quat& o) {
            mOrientation = o;
        }
        glm::quat GetOrientation()const { return mOrientation; }
        static void DestroyDescriptorPool(VkDevice device);
        ~GameObject();

        const std::string mName;
        const VkDevice mDevice;
        const VkBuffer mVertexBuffer;
        const VkBuffer mIndexBuffer;
        const uint16_t mNumberOfIndices;
        void CommitDataToObjectBuffer(uint32_t currentFrame);
        VkDescriptorSet GetDescriptorSet(uint32_t id) const {
            return helloObjectDescriptorSets[id];
        }
    private:
        glm::vec3 mPosition;
        glm::quat mOrientation;


        //size = MAX_FRAMES_IN_FLIGHT
        std::vector<VkBuffer> helloObjectUniformBuffer;
        std::vector<VkDeviceMemory> helloObjectUniformBufferMemory;
        std::vector<void*> helloObjectUniformBufferAddress;
        std::vector<VkDescriptorSet> helloObjectDescriptorSets;
    };
}