#include "my-vk.h"
#include <stdexcept>
#include "my-vk-extensions.h"
#include "my-vk-validationLayers.h"
#include <map>
#include <array>
#include <cassert>
#include <set>
#include "my-vk-shaders.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "object_namer.h"
#include <chrono>
#include "entities/game-object.h"
#include "io/asset-paths.h"
#include "entities/mesh.h"
#include "entities/image.h"
#include "concatenate.h"
VkApplicationInfo GetAppInfo() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello World";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "foobar";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    return appInfo;
}
//TODO: Move it somewhere more appropriate
VkVertexInputBindingDescription GetBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(entities::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}
//TODO: Move it somewhere more appropriate
std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    //inPosition
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(entities::Vertex, pos);
    //inUV0
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(entities::Vertex, uv0);
    //inColor
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(entities::Vertex, color);
    return attributeDescriptions;
}

void CreateHelloSampler(VkContext& ctx)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(ctx.physicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(ctx.device, &samplerInfo, nullptr, &ctx.helloSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    SET_NAME(ctx.helloSampler, VK_OBJECT_TYPE_SAMPLER, "HelloPipelineSampler");
}

void DestroyImageViews(VkContext& ctx) {
    for (VkImageView& img : ctx.swapChainImageViews) {
        vkDestroyImageView(ctx.device, img, nullptr);
    }
}

void CreateVkInstance(VkInstance& instance) {
    VkApplicationInfo appInfo = GetAppInfo();
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("fail to create vk instance");
    }
}


void CreateVkInstance(VkInstance& instance, const CustomAllocators& allocators) {
    //Evaluate if i can use my validation layers:
    if (EnableValidationLayers() && !CheckValidationLayerSupport())
        throw std::runtime_error("validation layers requested but not available");
    //Get the extensions for vk
    const std::vector<const char*> extensions = getRequiredExtensions(EnableValidationLayers());
    //build the info struct for instance
    VkApplicationInfo appInfo = GetAppInfo();
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    SetValidationLayerSupportAtInstanceCreateInfo(createInfo);

    VkAllocationCallbacks allocCallbacks = CreateCustomAllocatorsCallback(allocators);

    if (vkCreateInstance(&createInfo, &allocCallbacks , &instance) != VK_SUCCESS) {
        throw std::runtime_error("fail to create vk instance");
    }
}

void DestroyVkInstance(VkInstance instance)
{
    vkDestroyInstance(instance, nullptr);
}

void DestroyVkInstance(VkInstance instance, const CustomAllocators& allocators)
{
    auto cbks = CreateCustomAllocatorsCallback(allocators);
    vkDestroyInstance(instance, &cbks);
}

void SelectPhysicalDevice(VkContext& ctx)
{
    //how many devices?
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("no gpu suport vk");
    //Get them
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());
    std::multimap<int, VkPhysicalDevice> candidates;
    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);
        VkPhysicalDeviceFeatures features = {};
        vkGetPhysicalDeviceFeatures(device, &features);
        features.samplerAnisotropy = VK_TRUE;
        int score = 0;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;
        if (!features.geometryShader)
            score = -1000;
        candidates.insert(std::make_pair(score, device));
    }
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
        //Get the index of the graphics queue family
        auto graphicsQueueFamilyIdx = FindGraphicsQueueFamily(physicalDevice);
        ctx.graphicsFamily = *graphicsQueueFamilyIdx;
        assert(ctx.surface != nullptr);
        auto presentationQueueFamilyIdx = FindPresentationQueueFamily(physicalDevice,
            ctx.surface);
        ctx.presentFamily = *presentationQueueFamilyIdx;
    }
    else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    ctx.physicalDevice = physicalDevice;
}
std::optional<uint32_t> FindPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
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

std::optional<uint32_t> FindGraphicsQueueFamily(VkPhysicalDevice device)
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

void CreateLogicalQueue(VkContext& ctx, bool enableValidationLayers, std::vector<const char*> validationLayers = {})
{
    //for each unique queue family id we create a queue info
    std::set<uint32_t> uniqueQueueFamilies = { ctx.graphicsFamily, ctx.presentFamily };
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
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
    VkPhysicalDeviceFeatures deviceFeatures = {};
    vkGetPhysicalDeviceFeatures(ctx.physicalDevice, &deviceFeatures);
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    createInfo.pEnabledFeatures = &deviceFeatures;
    //The logical device extensions that i want
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, //swapchain
        VK_EXT_DEBUG_MARKER_EXTENSION_NAME //renderdoc marker
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>( deviceExtensions.size() );
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    //the layers enabled on this logical device
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }
    VkAllocationCallbacks allocatorsCallbacks = CreateCustomAllocatorsCallback(ctx.customAllocators);
    if (vkCreateDevice(ctx.physicalDevice, &createInfo, &allocatorsCallbacks, &ctx.device)) {
        throw std::runtime_error("Failed to create the logical device");
    }
    vkGetDeviceQueue(ctx.device, ctx.graphicsFamily, 0, &ctx.graphicsQueue);
    vkGetDeviceQueue(ctx.device, ctx.presentFamily, 0, &ctx.presentQueue);

    ctx.vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(ctx.device, "vkCmdDebugMarkerBeginEXT");
    ctx.vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(ctx.device, "vkCmdDebugMarkerEndEXT");
    ctx.vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(ctx.device, "vkCmdDebugMarkerInsertEXT");
}

void DestroyLogicalDevice(VkContext& ctx)
{
    VkAllocationCallbacks allocatorsCallbacks = CreateCustomAllocatorsCallback(ctx.customAllocators);
    vkDestroyDevice(ctx.device, &allocatorsCallbacks);
}

void CreateSurface(VkContext& ctx, GLFWwindow* window)
{
    VkAllocationCallbacks cbks = CreateCustomAllocatorsCallback(ctx.customAllocators);
    if (glfwCreateWindowSurface(ctx.instance, window, &cbks, &ctx.surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void DestroySurface(VkContext& ctx)
{
    VkAllocationCallbacks cbks = CreateCustomAllocatorsCallback(ctx.customAllocators);
    vkDestroySurfaceKHR(ctx.instance, ctx.surface, &cbks);
}

void CreateSwapChain(VkContext& ctx)
{
    SwapChainSupportDetails supportDetails = QuerySwapChainSupport(ctx.physicalDevice, ctx.surface);
    VkSurfaceFormatKHR chosenSurfaceFormat = ChooseSwapSurfaceFormat(supportDetails.formats);
    VkPresentModeKHR chosenPresentMode = ChooseSwapPresentMode(supportDetails.presentModes);
    if (supportDetails.capabilities.currentExtent.width != UINT32_MAX) {
        ctx.swapChainExtent = supportDetails.capabilities.currentExtent;
    }
    else {
        ctx.swapChainExtent = { 1024, 768 };
        ctx.swapChainExtent.width = std::max(supportDetails.capabilities.minImageExtent.width, 
            std::min(supportDetails.capabilities.maxImageExtent.width, ctx.swapChainExtent.width));
        ctx.swapChainExtent.height = std::max(supportDetails.capabilities.minImageExtent.height, 
            std::min(supportDetails.capabilities.maxImageExtent.height, ctx.swapChainExtent.height));
    }
    //sets the number of swap chain images to be either min+1 or the max number possible
    uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
    if (supportDetails.capabilities.maxImageCount > 0 &&
        imageCount > supportDetails.capabilities.maxImageCount) {
        imageCount = supportDetails.capabilities.maxImageCount;
    }
    //the swap chain properties
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = ctx.surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = chosenSurfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = ctx.swapChainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //this changes whether the queue families are different or not
    uint32_t queueFamilyIndices[] = { ctx.graphicsFamily, ctx.presentFamily };
    if (ctx.graphicsFamily != ctx.presentFamily) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    swapchainCreateInfo.preTransform = supportDetails.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = chosenPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(ctx.device, &swapchainCreateInfo, nullptr, &ctx.swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain!");
    }
    //now that we created the swap chain we get its images
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &imageCount, nullptr);
    ctx.swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, 
        &imageCount, ctx.swapchainImages.data());
    ctx.swapChainImageFormat = chosenSurfaceFormat.format;
}

void DestroySwapChain(VkContext& ctx)
{
    vkDestroySwapchainKHR(ctx.device, ctx.swapChain, nullptr);
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    //get the number of surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    //get the surface formats
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    //get the presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
            details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    //TODO: Support other image formats, like linear rgb
    throw new std::runtime_error("I only accept VK_FORMAT_B8G8R8A8_SRGB non linear for the swap chain framebuffers");
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    //TODO: prefer MAILBOX but if no MAILBOX use FIFO
    return VK_PRESENT_MODE_FIFO_KHR;
}

void CreateImageViewForSwapChain(VkContext& ctx)
{
    ctx.swapChainImageViews.resize(ctx.swapchainImages.size());
    for (size_t i = 0; i < ctx.swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = ctx.swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = ctx.swapChainImageFormat;
        //default color mapping
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        //the image purpouse and which part of it to be accessed
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(ctx.device, &createInfo, nullptr, 
            &ctx.swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

void CreateGraphicsPipeline(VkContext& ctx)
{
    LoadShaderModules(ctx);
    //description of the shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = ctx.shaderModules["hello_vertex"];
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = ctx.shaderModules["hello_fragment"];
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    //the dynamic states - they will have to set up in draw time
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    //vertex input description
    auto bindingDescription = GetBindingDescription();
    auto attributeDescriptions = GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    //input description: the geometry input will be triangles
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    //since the viewport and scissors are dynamic, is just need to tell the pipeline their number
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    //rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; //only relevant when doing shadow maps
    rasterizer.rasterizerDiscardEnable = VK_FALSE; //if true the geometry will never pass thru the rasterizer stage
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;
    //multisampling TODO: Disabled for now
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    //color blend
    //  per framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; //TODO: no color blending for now
    //  global
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE; //TODO: no color blending for now
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    //pipeline layout, to pass data to the shaders, sends nothing for now
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts = {
        ctx.helloCameraDescriptorSetLayout,//set 0
        ctx.helloObjectDescriptorSetLayout,//set 1
        ctx.helloSamplerDescriptorSetLayout //set 2
    };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()); 
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data(); 
    //we use no push constants for now
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, 
        nullptr, &ctx.helloPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    SET_NAME(ctx.helloPipelineLayout, 
        VK_OBJECT_TYPE_PIPELINE_LAYOUT, "HelloPipelineLayout");

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional
    //the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;    //add the shader stages
    //set the states
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    //link the pipeline layout to the pipeline
    pipelineInfo.layout = ctx.helloPipelineLayout;
    //link the render pass to the pipeline
    pipelineInfo.renderPass = ctx.mSwapchainRenderPass;
    pipelineInfo.subpass = 0;
    if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 
        1,//number of pipelines 
        &pipelineInfo, //list of pipelines (only one) 
        nullptr, &ctx.graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    SET_NAME(ctx.graphicsPipeline, VK_OBJECT_TYPE_PIPELINE, "HelloPipeline");
    vkDestroyShaderModule(ctx.device, ctx.shaderModules["hello_vertex"], nullptr);
    vkDestroyShaderModule(ctx.device, ctx.shaderModules["hello_fragment"], nullptr);
}

void LoadShaderModules(VkContext& ctx)
{
    auto vsPath = io::CalculatePathForShader("hello_shader_vert.spv");
    auto fsPath = io::CalculatePathForShader("hello_shader_frag.spv");
    auto vertShaderCode = ReadFile(vsPath);
    auto fragShaderCode = ReadFile(fsPath);
    VkShaderModule vert = CreateShaderModule(ctx.device, vertShaderCode);
    VkShaderModule frag = CreateShaderModule(ctx.device, fragShaderCode);
    ctx.shaderModules["hello_vertex"] = vert;
    ctx.shaderModules["hello_fragment"] = frag;
}

void DestroyPipelineLayout(VkContext& ctx)
{
    vkDestroyPipelineLayout(ctx.device, ctx.helloPipelineLayout, nullptr);
}

void CreateSwapchainRenderPass(VkContext& ctx)
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = entities::DepthBufferManager::findDepthFormat(ctx.physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //the image that'll hold the result
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = ctx.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//clear the values to a constant at the start
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store the rendered result in memory for later uses
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;//not using the stencil buffer
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//image presented in the swap chain
    //the reference to the image attachment that'll hold the result
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //the render subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    //the actual render pass, composed of image attachments and subpasses
    std::array<VkAttachmentDescription, 2> attachments = { 
        colorAttachment, 
        depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    //the render pass depends upon the previous render pass to run
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    //wait until the image is attached
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.mSwapchainRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create on screen render pass!");
    }
    SET_NAME(ctx.mSwapchainRenderPass, VK_OBJECT_TYPE_RENDER_PASS, "main render pass");
}

void DestroySwapchainRenderPass(VkContext& ctx)
{
    vkDestroyRenderPass(ctx.device, ctx.mSwapchainRenderPass, nullptr);
}

void CreateRenderToTextureRenderPass(VkContext& ctx)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;  // Format of the offscreen image
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};  // If depth is needed
    depthAttachment.format = entities::DepthBufferManager::findDepthFormat(ctx.physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;  // Index of the attachment in the render pass
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;  // Index of the depth attachment
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;  // The color attachment reference
    subpass.pDepthStencilAttachment = &depthAttachmentRef;  // The depth attachment reference

    std::array<VkAttachmentDescription, 2> attachments = {
    colorAttachment,
    depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // Operations outside the render pass
    dependency.dstSubpass = 0;  // The first (and only) subpass in the render pass
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;  // No prior access needed
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.mRenderToTextureRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render-to-texture render pass!");
    }
    SET_NAME(ctx.mSwapchainRenderPass, VK_OBJECT_TYPE_RENDER_PASS, "render-to-texture render pass");
}

void DestroyPipeline(VkContext& ctx)
{
    vkDestroyPipeline(ctx.device, ctx.graphicsPipeline, nullptr);
}

void CreateFramebuffers(VkContext& ctx, VkImageView depthImageView)
{
    //one framebuffer for each image. ImageViews are the interface to the underlying images
    ctx.swapChainFramebuffers.resize(ctx.swapChainImageViews.size());
    for (size_t i = 0; i < ctx.swapChainImageViews.size(); i++) {
        std::vector<VkImageView> attachments = {
            ctx.swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = ctx.mSwapchainRenderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = ctx.swapChainExtent.width;
        framebufferInfo.height = ctx.swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, 
            &ctx.swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
        auto _name = Concatenate("swapchain framebuffer ", i);
        SET_NAME(ctx.swapChainFramebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, _name.c_str());
    }
}

void DestroyFramebuffers(VkContext& ctx)
{
    for (auto fb : ctx.swapChainFramebuffers) {
        vkDestroyFramebuffer(ctx.device, fb, nullptr);
    }
}

void CreateCommandPool(VkContext& ctx)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = ctx.graphicsFamily;
    if (vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
    SET_NAME(ctx.commandPool, VK_OBJECT_TYPE_COMMAND_POOL, "MainCommandPool");
}

void DestroyCommandPool(VkContext& ctx)
{
    vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
}

void CreateCommandBuffer(VkContext& ctx)
{
    ctx.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)ctx.commandBuffers.size();

    if (vkAllocateCommandBuffers(ctx.device, &allocInfo, ctx.commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

bool BeginFrame(VkContext& ctx, uint32_t& imageIndex) {
    // check window area to deal with the degenerate case of the user dragging a border until
    // it becomes zero
    int width = 0, height = 0;
    glfwGetFramebufferSize(ctx.window, &width, &height);
    bool zeroArea = (width == 0) || (height == 0);
    if (zeroArea)
        return false;

    //Fences block cpu, waiting for result. So we wait for the previous frame to finish
    vkWaitForFences(ctx.device, 1, &ctx.inFlightFences[ctx.currentFrame], VK_TRUE, UINT64_MAX);
    //get an image from the swap chain
    VkResult result = vkAcquireNextImageKHR(ctx.device, ctx.swapChain, UINT64_MAX,
        ctx.imageAvailableSemaphores[ctx.currentFrame], //this semaphore will be signalled when the presentation is done with this image
        VK_NULL_HANDLE, //no fence cares  
        &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain(ctx);
        return false;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    //if the swapchain is ok we can reset the fence
    vkResetFences(ctx.device, 1, &ctx.inFlightFences[ctx.currentFrame]);
    //resets the command buffer
    vkResetCommandBuffer(ctx.commandBuffers[ctx.currentFrame], 0);
    //begin the command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(ctx.commandBuffers[ctx.currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    //begin the render pass of the render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = ctx.mSwapchainRenderPass;
    renderPassInfo.framebuffer = ctx.swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = ctx.swapChainExtent;
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(ctx.commandBuffers[ctx.currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    //bind the pipeline
    vkCmdBindPipeline(ctx.commandBuffers[ctx.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.graphicsPipeline);
    //viewport and scissors are dynamic
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(ctx.swapChainExtent.width);
    viewport.height = static_cast<float>(ctx.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(ctx.commandBuffers[ctx.currentFrame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = ctx.swapChainExtent;
    vkCmdSetScissor(ctx.commandBuffers[ctx.currentFrame], 0, 1, &scissor);
    return true;
}

void EndFrame(VkContext& ctx, uint32_t currentImageIndex) {
    //end the render .pass
    vkCmdEndRenderPass(ctx.commandBuffers[ctx.currentFrame]);
    //end the command buffer
    if (vkEndCommandBuffer(ctx.commandBuffers[ctx.currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
    //submits the queue:
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // it'll wait until the colours are written to the framebuffer
    VkSemaphore waitSemaphores[] = { ctx.imageAvailableSemaphores[ctx.currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &ctx.commandBuffers[ctx.currentFrame];

    VkSemaphore signalSemaphores[] = { ctx.renderFinishedSemaphores[ctx.currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, ctx.inFlightFences[ctx.currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    //presentation
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { ctx.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &currentImageIndex;

    VkResult result = vkQueuePresentKHR(ctx.presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || ctx.framebufferResized) {
        ctx.framebufferResized = false;
        RecreateSwapChain(ctx);
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    ctx.currentFrame = (ctx.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void SetMark(std::array<float, 4> color, 
    std::string name,
    VkCommandBuffer cmd,
    VkContext ctx) {
    VkDebugMarkerMarkerInfoEXT markerInfo = {};
    markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    markerInfo.pNext = nullptr;
    markerInfo.pMarkerName = name.c_str();
    markerInfo.color[0] = color[0];              // Optional RGBA color
    markerInfo.color[1] = color[1];
    markerInfo.color[2] = color[2];
    markerInfo.color[3] = color[3];
    ctx.vkCmdDebugMarkerBeginEXT(cmd, &markerInfo);
}

void DrawGameObject(entities::GameObject* go, CameraUniformBuffer& camera, VkContext& ctx)
{
    VkCommandBuffer cmdBuffer = ctx.commandBuffers[ctx.currentFrame];
    SetMark({ 1.0f, 0.0f, 0.0f, 1.0f }, go->mName, cmdBuffer, ctx);
    //copies camera data to gpu
    memcpy(ctx.helloCameraUniformBufferAddress[ctx.currentFrame], &camera, sizeof(camera));
    //copies object data to gpu
    go->CommitDataToObjectBuffer(ctx.currentFrame);
    go->mMesh->Bind(cmdBuffer);
    //bind the camera descriptor set
    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,   // We assume it's a graphics pipeline
        ctx.helloPipelineLayout,                    // The pipeline layout that matches the shader's descriptor set layouts
        0,                                 // firstSet, which is the index of the first descriptor set (set = 0)
        1,                                 // descriptorSetCount, number of descriptor sets to bind
        &ctx.helloCameraDescriptorSets[ctx.currentFrame],              // Pointer to the array of descriptor sets (only one in this case)
        0,                                 // dynamicOffsetCount, assuming no dynamic offsets
        nullptr                            // pDynamicOffsets, assuming no dynamic offsets
    );
    //bind the object-specific descriptor set
    // Bind the object-specific descriptor set (set = 1)
    VkDescriptorSet objectDescriptorSet = go->GetDescriptorSet(ctx.currentFrame);
    uint32_t dynamicOffset = go->DynamicOffset(ctx.currentFrame);
    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ctx.helloPipelineLayout,
        1,                                 // firstSet, index of the first descriptor set (set = 1)
        1,                                 // descriptorSetCount, number of descriptor sets to bind
        &objectDescriptorSet,              // Pointer to the array of descriptor sets (only one in this case)
        1,
        &dynamicOffset
    );
    //bind the sampler
    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ctx.helloPipelineLayout,
        2, //third set
        1,
        &ctx.helloSamplerDescriptorSets[ctx.currentFrame],
        0,
        nullptr
    );
    //Draw command
    vkCmdDrawIndexed(cmdBuffer,
        static_cast<uint32_t>(go->mMesh->NumberOfIndices()), 
        1, 
        0, 
        0, 
        0);
    ctx.vkCmdDebugMarkerEndEXT(cmdBuffer);
}

void CreateSyncObjects(VkContext& ctx)
{
    ctx.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    ctx.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    ctx.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//gotcha: if the fence is unsignalled DrawFrame will hang waiting for it to be signalled during the 1st execution
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceInfo, nullptr, &ctx.inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

}

void DestroySyncObjects(VkContext& ctx)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(ctx.device, ctx.imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(ctx.device, ctx.renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(ctx.device, ctx.inFlightFences[i], nullptr);
    }
}

void RecreateSwapChain(VkContext& ctx)
{
    assert(false);//broken
    //TODO: depthBufferManager need to recreate it's depth buffers
    //handles minimization
    int w = 0, h = 0;
    glfwGetFramebufferSize(ctx.window, &w, &h);
    while (w == 0 || h == 0) {

        glfwGetFramebufferSize(ctx.window, &w, &h);
        glfwWaitEvents();
    }
    //halt until the device is idle, the resources may still be in use
    vkDeviceWaitIdle(ctx.device);
    //cleanup
    DestroyFramebuffers(ctx);
    DestroyImageViews(ctx);
    DestroySwapChain(ctx);
    //recreation
    CreateSwapChain(ctx);
    CreateImageViewForSwapChain(ctx);
    //CreateFramebuffers(ctx);
}


uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkContext ctx) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer,
    VkDeviceMemory& bufferMemory, VkContext& ctx)
{
    assert(size != 0);//size must not be zero
    //Buffer description
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //create the buffer
    if (vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    //the memory the buffer will require, not necessarely equals to the size of the data being stored
    //in it.
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.device, buffer, &memRequirements);
    //TODO: Use VulkanMemoryAllocator instead of allocating memory on my own because gpus have a limited number of simultaneous memory allocations.
    //In a scene with a lot of objects this limit is reached very fast
    //https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, ctx);
    //Allocate the memory
    if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(ctx.device, buffer, bufferMemory, 0);
}

void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkContext& ctx)
{
    //a temporary command buffer for copy
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = ctx.commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);
    //begin recording commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //this command buffer will be used only once
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    //copy command
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    //end the recording
    vkEndCommandBuffer(commandBuffer);
    //submit the command
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(ctx.graphicsQueue, //graphics queue can handle copy commands just fine 
        1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.graphicsQueue);//cpu waits until copy is done
    //free the command buffer
    vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}
void CreateDescriptorSetLayoutForSampler(VkContext& ctx) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional
    VkDescriptorSetLayoutCreateInfo samplerLayoutInfo{};
    samplerLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    samplerLayoutInfo.bindingCount = 1;
    samplerLayoutInfo.pBindings = &samplerLayoutBinding;
    if (vkCreateDescriptorSetLayout(ctx.device, &samplerLayoutInfo,
        nullptr, &ctx.helloSamplerDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout for object");
    }
    SET_NAME(ctx.helloObjectDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        "SamplerDescriptorSetLayout");
}
void CreateDescriptorSetLayoutForObject(VkContext& ctx)
{
    VkDescriptorSetLayoutBinding objectLayoutBinding{};
    objectLayoutBinding.binding = 0;
    objectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    objectLayoutBinding.descriptorCount = 1;
    objectLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
    objectLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
    objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    objectLayoutInfo.bindingCount = 1;
    objectLayoutInfo.pBindings = &objectLayoutBinding;

    if (vkCreateDescriptorSetLayout(ctx.device, &objectLayoutInfo,
        nullptr, &ctx.helloObjectDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout for object");
    }
    SET_NAME(ctx.helloObjectDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        "ObjectsDescriptorSetLayout");
}

void CreateDescriptorSetLayoutForCamera(VkContext& ctx)
{
    VkDescriptorSetLayoutBinding cameraLayoutBinding{};
    cameraLayoutBinding.binding = 0;
    cameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraLayoutBinding.descriptorCount = 1;
    cameraLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cameraLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
    cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cameraLayoutInfo.bindingCount = 1;
    cameraLayoutInfo.pBindings = &cameraLayoutBinding;

    if (vkCreateDescriptorSetLayout(ctx.device, &cameraLayoutInfo, 
        nullptr, &ctx.helloCameraDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    SET_NAME(ctx.helloCameraDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "CameraDescriptorSetLayout");
}

void DestroyDescriptorSets(VkContext& ctx)
{
    vkDestroyDescriptorSetLayout(ctx.device, ctx.helloCameraDescriptorSetLayout, nullptr);
}

void CreateUniformBuffersForCamera(VkContext& ctx)
{
    VkDeviceSize bufferSize = sizeof(CameraUniformBuffer);

    ctx.helloCameraUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
    ctx.helloCameraUniformBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
    ctx.helloCameraUniformBufferAddress.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateBuffer(bufferSize, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, //will be used as uniform buffer
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //visible to both gpu and cpu
            ctx.helloCameraUniformBuffer[i], ctx.helloCameraUniformBufferMemory[i], ctx);
        //map the memory, forever    
        vkMapMemory(ctx.device, ctx.helloCameraUniformBufferMemory[i], 0,
            bufferSize, 0, &ctx.helloCameraUniformBufferAddress[i]);
        auto n0 = Concatenate("CameraBufferMemory", i);
        SET_NAME(ctx.helloCameraUniformBufferMemory[i], VK_OBJECT_TYPE_DEVICE_MEMORY, n0.c_str());
        auto n1 = Concatenate("CameraUniformBuffer", i);
        SET_NAME(ctx.helloCameraUniformBuffer[i], VK_OBJECT_TYPE_BUFFER, n1.c_str());
    }
}

/// <summary>
/// To create descriptor sets i need to create descriptor pools to provide them. The descriptor sets
/// are created using the layouts, from the descriptor pools.
/// </summary>
/// <param name="ctx"></param>
void CreateDescriptorPool(VkContext& ctx)
{
    //camera descriptor pool - there will be only one camera
    VkDescriptorPoolSize cameraPoolSize{};
    cameraPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraPoolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo cameraPoolInfo{};
    cameraPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    cameraPoolInfo.poolSizeCount = 1;
    cameraPoolInfo.pPoolSizes = &cameraPoolSize;
    cameraPoolInfo.maxSets = 1 * MAX_FRAMES_IN_FLIGHT; // Only one camera. NEVER FORGET TO MULTIPLY BY MAX_FRAMES_IN_

    if (vkCreateDescriptorPool(ctx.device, &cameraPoolInfo, nullptr, &ctx.helloCameraDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create camera descriptor pool!");
    }
    SET_NAME(ctx.helloCameraDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "helloCameraDescriptorPool");
    //////CREATES THE DESCRIPTOR POOL FOR SAMPLERS//////
    VkDescriptorPoolSize samplerPoolSize{};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo samplerPoolInfo{};
    samplerPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    samplerPoolInfo.poolSizeCount = 1;
    samplerPoolInfo.pPoolSizes = &samplerPoolSize;
    samplerPoolInfo.maxSets = 1 * MAX_FRAMES_IN_FLIGHT; // Only one sampler. NEVER FORGET TO MULTIPLY BY MAX_FRAMES_IN_

    if (vkCreateDescriptorPool(ctx.device, &samplerPoolInfo, nullptr, &ctx.helloSamplerDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create sampler descriptor pool!");
    }
    SET_NAME(ctx.helloSamplerDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "helloSamplerDescriptorPool");

}

void CreateDescriptorSetsForCamera(VkContext& ctx)
{
    assert(ctx.helloCameraDescriptorPool != VK_NULL_HANDLE);
    //one layout for each frame in flight, create the vector filling with the layout that i alredy have
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, 
        ctx.helloCameraDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = ctx.helloCameraDescriptorPool;
    //one descriptor set for each frame in flight
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    ctx.helloCameraDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(ctx.device, &allocInfo, ctx.helloCameraDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets for camera!");
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ctx.helloCameraUniformBuffer[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraUniformBuffer);
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = ctx.helloCameraDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);
    }

}

void CreateDescriptorSetsForSampler(VkContext& ctx, entities::GpuTextureManager* manager, const std::string& name)
{
    assert(ctx.helloSamplerDescriptorPool != VK_NULL_HANDLE);
    //one layout for each frame in flight, create the vector filling with the layout that i alredy have
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
        ctx.helloSamplerDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = ctx.helloSamplerDescriptorPool;
    //one descriptor set for each frame in flight
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    ctx.helloSamplerDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(ctx.device, &allocInfo, 
        ctx.helloSamplerDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets for SAMPLER!");
    }
    VkImageView view = manager->GetImageView(name);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = view;
        imageInfo.sampler = ctx.helloSampler;
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = ctx.helloSamplerDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);
    }
}

void VkContext::DestroyCameraBuffer(VkContext& ctx)
{
    for (auto& b : ctx.helloCameraUniformBuffer) {
        vkDestroyBuffer(ctx.device, b, nullptr);
    }
    for (auto& b : ctx.helloCameraUniformBufferMemory) {
        vkFreeMemory(ctx.device, b, nullptr);
    }
    vkDestroyDescriptorPool(ctx.device, ctx.helloCameraDescriptorPool, nullptr);
}
