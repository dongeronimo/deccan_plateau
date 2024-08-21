#pragma once
#include <vector>
#include <vulkan/vulkan.h>
/// <summary>
/// List all extensions available
/// </summary>
/// <returns></returns>
std::vector<VkExtensionProperties> GetExtensions();
/// <summary>
/// List of required extensions
/// </summary>
std::vector<const char*> getRequiredExtensions(bool enableValidationLayers);
