#include "my-device.h"
#include <cassert>
#include <set>
#include <vector>
#include "utils/object_namer.h"
#include <stdexcept>
namespace myvk {
    Device* Device::gDevice;

    Device::Device(VkPhysicalDevice physicalDevice, VkInstance instance, VkSurfaceKHR surface, std::vector<const char*> validationLayers)
        :mPhysicalDevice(physicalDevice), mInstance(instance), mSurface(surface)
    {
        assert(physicalDevice != VK_NULL_HANDLE);
        assert(instance != VK_NULL_HANDLE);
        assert(surface != VK_NULL_HANDLE);
        assert(gDevice == nullptr);
        gDevice = this;
        //Get the queue families. They can be equal, and in that case that means that
        //the queue can do both presentation and graphics
        auto graphicsQueueFamilyIdx = FindGraphicsQueueFamily(physicalDevice);
        auto presentationQueueFamilyIdx = FindPresentationQueueFamily(physicalDevice,surface);
        this->mGraphicsQueueFamily = *graphicsQueueFamilyIdx;
        this->mPresentationQueueFamily = *presentationQueueFamilyIdx;
        //for each unique queue family id we create a queue info
        std::set<uint32_t> uniqueQueueFamilies = { mGraphicsQueueFamily, mPresentationQueueFamily };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo  queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            float queuePriority = 1.0f;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        //description of the logical device
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        VkPhysicalDeviceFeatures deviceFeatures = {};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        //The logical device extensions that i want
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, //swapchain
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME //renderdoc marker
        };
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //the layers enabled on this logical device
        if (validationLayers.size() > 0) {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            deviceCreateInfo.enabledLayerCount = 0;
        }
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &mDevice);
        vkGetDeviceQueue(mDevice, mGraphicsQueueFamily, 0, &mGraphicsQueue);
        vkGetDeviceQueue(mDevice, mPresentationQueueFamily, 0, &mPresentationQueue);
        /*vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDebugMarkerBeginEXT");
        vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDebugMarkerEndEXT");
        vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDebugMarkerInsertEXT");
        *///initializes the object namer
        vk::ObjectNamer::Instance().Init();
        //creates the command pool
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = mGraphicsQueueFamily;
        if (vkCreateCommandPool(mDevice, 
            &commandPoolCreateInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
        SET_NAME(mCommandPool, VK_OBJECT_TYPE_COMMAND_POOL, "MainCommandPool");
    }
    Device::~Device()
    {
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        vkDestroyDevice(mDevice, nullptr);
    }
    std::optional<uint32_t> Device::FindGraphicsQueueFamily(VkPhysicalDevice device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        std::optional<uint32_t> result;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                result = i;
            }
            i++;
        }
        return result;
    }
    std::optional<uint32_t> Device::FindPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        std::optional<uint32_t> result;
        for (const auto& queueFamily : queueFamilies)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                result = i;
                break;
            }
            i++;
        }
        return result;
    }
}