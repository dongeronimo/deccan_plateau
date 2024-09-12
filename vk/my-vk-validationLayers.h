#pragma once
#include <vulkan/vulkan.h>
#include "vk/my-vk.h"
#include <vector>

std::vector<const char*> GetValidationLayerNames();

bool EnableValidationLayers();

bool CheckValidationLayerSupport();

void SetValidationLayerSupportAtInstanceCreateInfo(VkInstanceCreateInfo& info);

void SetupDebugMessenger(VkInstance instance, 
    VkDebugUtilsMessengerEXT& debugMessenger,
    const CustomAllocators& allocators);

void SetupDebugMessenger(VkInstance instance,
    VkDebugUtilsMessengerEXT& debugMessenger);


void DestroyDebugMessenger(VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const CustomAllocators& allocators);

void DestroyDebugMessenger(VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger);