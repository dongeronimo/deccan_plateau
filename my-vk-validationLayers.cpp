#include "my-vk-validationLayers.h"
#include <stdexcept>
#ifdef NDEBUG
const bool gEnableValidationLayers = false;
#else
const bool gEnableValidationLayers = true;
#endif
//Table of validation layers that i'll use
const std::vector<const char*> gValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
/// <summary>
/// Debug callback
/// </summary>
/// <param name="messageSeverity"></param>
/// <param name="messageType"></param>
/// <param name="pCallbackData"></param>
/// <param name="pUserData"></param>
/// <returns></returns>
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    if (messageSeverity >= VK_DEBUG_LEVEL)
    {
        printf("Validation Layer:\n "
               "  %s:%d  %s\n",
               //"  msg:%s name:%s\n ", 
            pCallbackData->pMessageIdName,
            pCallbackData->messageIdNumber,
            pCallbackData->pMessage
            );
    }
    return VK_FALSE;
}


std::vector<const char*> GetValidationLayerNames()
{
    return gValidationLayers;
}

bool EnableValidationLayers()
{
    return gEnableValidationLayers;
}

bool CheckValidationLayerSupport()
{
    //how many layers?
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    //get them
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    //all validation layers that i want are present (they are defined at gValidationLayers)
    for (const char* layerName : gValidationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProps : availableLayers)
        {
            if (strcmp(layerName, layerProps.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            return false;
        }
    }
    return true;
}

void SetValidationLayerSupportAtInstanceCreateInfo(VkInstanceCreateInfo& info)
{
    if (gEnableValidationLayers)
    {
        info.enabledLayerCount = static_cast<uint32_t>(gValidationLayers.size());
        info.ppEnabledLayerNames = gValidationLayers.data();
    }
    else {
        info.enabledLayerCount = 0;
    }
}

void SetupDebugMessenger(VkInstance instance, 
    VkDebugUtilsMessengerEXT& debugMessenger, 
    const CustomAllocators& allocators)
{
    //the debug utils description
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional
    //vkCreateDebugUtilsMessengerEXT is an extension, i have to get it's pointer manually
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, 
        "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        VkAllocationCallbacks allocCallbacks = CreateCustomAllocatorsCallback(allocators);
        VkResult result = func(instance, &createInfo, &allocCallbacks, &debugMessenger);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Could not create debug-utils messenger");
    }
    else
    {
        throw std::runtime_error("Failed to load the pointer to vkCreateDebugUtilsMessengerEXT");
    }
}

void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const CustomAllocators& allocators)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    VkAllocationCallbacks allocCallbacks = CreateCustomAllocatorsCallback(allocators);
    if (func != nullptr) {
        func(instance, debugMessenger, &allocCallbacks);
    }
}
