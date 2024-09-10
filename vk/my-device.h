#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>

namespace myvk {
    /// <summary>
    /// Holds the vulkan's device and it's queues. Should be initialized just
    /// once because the first time it's initialized it fills gDevice so that 
    /// i have easy access to it everywhere.
    /// </summary>
    class Device {
    public:
        static Device* gDevice;
        /// <summary>
        /// Creates the device
        /// - Fills gDevice.
        /// - Initializes the VkDevice, its queues and command pool
        /// - Initializes the object namer that is used for debugging
        /// </summary>
        /// <param name="physicalDevice"></param>
        /// <param name="instance"></param>
        /// <param name="surface"></param>
        /// <param name="validationLayers"></param>
        Device(VkPhysicalDevice physicalDevice, 
            VkInstance instance, 
            VkSurfaceKHR surface,
            std::vector<const char*> validationLayers);
        /// <summary>
        /// This destructor should be called after everything else but the 
        /// instance has been destroyed.
        /// </summary>
        ~Device();
        VkCommandPool GetCommandPool()const { return mCommandPool; }
        VkDevice GetDevice()const { return mDevice; }
        uint32_t GetGraphicsQueueFamily()const { return mGraphicsQueueFamily; }
        uint32_t GetPresentationQueueFamily()const { return mPresentationQueueFamily; }
        VkQueue GetGraphicsQueue()const { return mGraphicsQueue; }
        VkQueue GetPresentationQueue()const { return mPresentationQueue; }

    private:
        const VkPhysicalDevice mPhysicalDevice;
        const VkInstance mInstance;
        const VkSurfaceKHR mSurface;
        VkDevice mDevice;
        VkQueue mGraphicsQueue;
        VkQueue mPresentationQueue;
        uint32_t mGraphicsQueueFamily;
        uint32_t mPresentationQueueFamily;
        VkCommandPool mCommandPool;
        std::optional<uint32_t> FindGraphicsQueueFamily(VkPhysicalDevice device);
        std::optional<uint32_t> FindPresentationQueueFamily(VkPhysicalDevice device, 
            VkSurfaceKHR surface);
    };
}