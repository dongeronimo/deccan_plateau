#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/quaternion.hpp>
#include <array>
#define MAX_NUMBER_OF_GAME_OBJECTS 100
struct VkContext;
namespace entities {
    class Mesh;
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
    
    /// <summary>
    /// This class holds the pool of uniform buffers for game objects. The size of each entry in the pool is MAX_NUMBER_OF_GAME_OBJECTS
    /// and gObjectDescriptorPool must take that size into account too. Also take into account that because we have multiple frames in
    /// flight (MAX_FRAMES_IN_FLIGHT) there must be a buffer for each frame in flight to not mix data from one frame into other
    /// </summary>
    class GameObjectUniformBufferPool {
    public:
        static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkContext ctx);
        GameObjectUniformBufferPool(VkContext* ctx);
        ~GameObjectUniformBufferPool();

        static std::pair<uintptr_t, VkDescriptorSet> Get(uint32_t i);
        /// <summary>
        /// Call this before creating any uniform buffer. It initializes the pool
        /// </summary>
        /// <param name="ctx"></param>
        static void Initialize(VkContext* ctx);

        static uintptr_t BaseAddress();
        /// <summary>
        /// Destroy the pool. Since it depends upon a valid VkDevice do that before destroying the device
        /// </summary>
        static void Destroy();
    private:
        const VkContext* mCtx;
        void* mBaseAddress;
        VkDescriptorPool mObjectDescriptorPool = VK_NULL_HANDLE;
        VkBuffer mBigBufferForDeviceMemoryAllocation = VK_NULL_HANDLE;
        VkDeviceMemory mBuffersMemory = VK_NULL_HANDLE;

        std::array<uintptr_t, MAX_FRAMES_IN_FLIGHT* MAX_NUMBER_OF_GAME_OBJECTS> mUniformBufferOffsets;
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT* MAX_NUMBER_OF_GAME_OBJECTS> mDescriptorSets;
    };
    class GameObject {
    public:
        GameObject(VkContext* ctx, 
            const std::string& name, 
            const Mesh* mesh);
        void SetPosition(glm::vec3& pos) {
            mPosition = pos;
        }
        glm::vec3 GetPosition()const { return mPosition; }
        void SetOrientation(glm::quat& o) {
            mOrientation = o;
        }
        glm::quat GetOrientation()const { return mOrientation; }
        ~GameObject();
        uint32_t DynamicOffset(uint32_t frame)const {
            return (mId * MAX_FRAMES_IN_FLIGHT + frame) * sizeof(ObjectUniformBuffer);
        }
        const Mesh* mMesh;
        const uint32_t mId;
        const glm::vec3 mPickerColor;
        const std::string mName;
        const VkDevice mDevice;
        void CommitDataToObjectBuffer(uint32_t currentFrame);
        VkDescriptorSet GetDescriptorSet(uint32_t id) const {
            return helloObjectDescriptorSets[id];
        }
    private:
        glm::vec3 mPosition;
        glm::quat mOrientation;
        std::array<uintptr_t, MAX_FRAMES_IN_FLIGHT> helloObjectUniformBufferAddress;
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> helloObjectDescriptorSets;
    };
}