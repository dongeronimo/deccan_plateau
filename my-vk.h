#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <GLFW/glfw3.h>
#include <map>
#include <array>
#include <glm/glm.hpp>
/// <summary>
/// Stores the pointers to custom allocators for VK. Custom allocators are not
/// striclty necessary but i'd like to let the hooks for using them in the code.
/// </summary>
struct CustomAllocators
{
    void* (*myAllocationFunction)(void* pUserData, 
        size_t size, 
        size_t alignment, 
        VkSystemAllocationScope allocationScope);
    void* (*myReallocationFunction)(void* pUserData,
        void* pOriginal,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope allocationScope);
    void (*myFreeFunction)(
        void* pUserData,
        void* pMemory);
};

inline VkAllocationCallbacks CreateCustomAllocatorsCallback(const CustomAllocators& allocators)
{
    VkAllocationCallbacks allocCallbacks{};
    allocCallbacks.pUserData = nullptr; // Optional user data
    allocCallbacks.pfnAllocation = allocators.myAllocationFunction;
    allocCallbacks.pfnReallocation = allocators.myReallocationFunction;
    allocCallbacks.pfnFree = allocators.myFreeFunction;
    return allocCallbacks;
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
//Uniform buffer struct, equal to the one in the shader (ubo, binding 0)
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

//TODO: Move it somewhere more appropriate
//The vertex data structure, for a 2d vertex and it's color
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};
//TODO: Move it somewhere more appropriate
//Returns the vertex binding, it'll be one binding, with input per-vertex and stride equals to the
//size of Vertex; 
VkVertexInputBindingDescription GetBindingDescription();
//TODO: Move it somewhere more appropriate
//Describes the 2 attributes that we have in the shader, inPosition and inColor
std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
/// <summary>
/// Kitchen sink will all vk data.
/// </summary>
struct VkContext
{
    GLFWwindow* window;
    /// <summary>
    /// TODO: Allow for many custom allocators, one for each situation
    /// Holds the function pointer for the custom allocator callbacks
    /// </summary>
    CustomAllocators customAllocators;
    /// <summary>
    /// The Vk library instance, the first thing to be created and the last to be
    /// destroyed. Thru it we get the physical devices.
    /// </summary>
    VkInstance instance;
    /// <summary>
    /// Debug messenger obj
    /// </summary>
    VkDebugUtilsMessengerEXT debugMessenger;
    /// <summary>
    /// The physical device: the representation of the physical gpu
    /// </summary>
    VkPhysicalDevice physicalDevice;
    /// <summary>
    /// Id of the graphics queue family in physicalDevice. Can be the same as presentFamily
    /// </summary>
    uint32_t graphicsFamily;
    /// <summary>
    /// Id of the present queue family in physicalDevice. Can be the same as graphicsFamily
    /// </summary>
    uint32_t presentFamily;
    /// <summary>
    /// The logical device, that we use to interact with the gpu
    /// </summary>
    VkDevice device;
    /// <summary>
    /// Graphics command queue. Can be the equal to presentQueue, which means that the queue supports
    /// both categories of commands
    /// </summary>
    VkQueue graphicsQueue;
    /// <summary>
    /// THe surface is the glue between the window system and vk.
    /// </summary>
    VkSurfaceKHR surface;
    /// <summary>
    /// Presentation command queue. Can be equal to graphicsQueue.
    /// </summary>
    VkQueue presentQueue;
    /// <summary>
    /// size of the swap chain images - normally will be the size of the window
    /// </summary>
    VkExtent2D swapChainExtent;
    /// <summary>
    /// How many images in the swap chain?
    /// </summary>
    uint32_t imageCount;
    /// <summary>
    /// Swap chain object
    /// </summary>
    VkSwapchainKHR swapChain;
    /// <summary>
    /// Table of images, created by the swap chain
    /// </summary>
    std::vector<VkImage> swapchainImages;
    /// <summary>
    /// Stores the image format that i chose
    /// </summary>
    VkFormat swapChainImageFormat;
    /// <summary>
    /// The image views for the swap chain images
    /// </summary>
    std::vector<VkImageView> swapChainImageViews;
    /// <summary>
    /// Table of swap chain framebuffers, one for each swap chain image
    /// </summary>
    std::vector<VkFramebuffer> swapChainFramebuffers;
    /// <summary>
    /// My table of shader modules, they are thin wrappers around the .spv
    /// </summary>
    std::map<std::string, VkShaderModule> shaderModules;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    /// <summary>
    /// The pipeline object. At the moment I have only one material (shaders + fixed states configs) so
    /// i have only one pipeline. In the future if i need more materials then i'll need more pipelines bc
    /// they are static and i can't change their shaders and fixed function properties once created.
    /// </summary>
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    /// <summary>
    /// Table of command buffers, one for each frame in flight (not one for each swap chain image)
    /// </summary>
    std::vector<VkCommandBuffer> commandBuffers;

    std::array<float, 4> clearColor;
    /// <summary>
    /// These semaphores signal when the image is available for the rendering process, blocking
    /// the gpu progress until the image is available
    /// </summary>
    std::vector<VkSemaphore> imageAvailableSemaphores;
    /// <summary>
    /// These ones signal when the render is finished;
    /// </summary>
    std::vector<VkSemaphore> renderFinishedSemaphores;
    /// <summary>
    /// Fences block the CPU until the gpu is done. We have to wait until the frame in flight is ready
    /// </summary>
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;
    //TODO: create a single, big ass VkDeviceMemory and do linear allocation of vertex and index buffers
    //TODO: merge the vertex buffer memory and the index buffer memory into a single memory.
    VkBuffer vertexBuffer; //TODO:  move to some kind of mesh object
    VkDeviceMemory vertexBufferMemory; //TODO: move to some kind of mesh object
    VkBuffer indexBuffer; //TODO: move this to some kind of mesh object
    VkDeviceMemory indexBufferMemory; //TODO: move this to some kind of mesh object
    VkDescriptorSetLayout descriptorSetLayout;//TODO: this info is coupled to the shader and to the pipeline so it should be part of the material

    //One buffer for each frame in flight
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory; //TODO: there should be a big ass buffer for the uniforms, separated from the big ass buffer for the mesh data
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};

void DestroyImageViews(VkContext& ctx);
/// <summary>
/// Creates an instance without using custom allocators. The first step to use VK is to 
/// create an instance.
/// </summary>
/// <param name="instance"></param>
void CreateVkInstance(VkInstance& instance);
/// <summary>
/// Creates an instance using custom allocators.The first step to use VK is to 
/// create an instance.
/// </summary>
/// <param name="instance"></param>
void CreateVkInstance(VkInstance& instance, const CustomAllocators& allocators);
/// <summary>
/// If you create the instance using  CreateVkInstance(VkInstance& instance) you
/// have to use this one 
/// </summary>
/// <param name="instance"></param>
void DestroyVkInstance(VkInstance instance);
/// <summary>
/// If you create the instance using   CreateVkInstance(VkInstance& instance, const CustomAllocators& allocators) you
/// have to use this one 
/// <summary>
void DestroyVkInstance(VkInstance instance, const CustomAllocators& allocators);

void SelectPhysicalDevice(VkContext& ctx);
 
std::optional<uint32_t> FindGraphicsQueueFamily(VkPhysicalDevice device);

void CreateLogicalQueue(VkContext& ctx, bool enableValidationLayers, std::vector<const char*> validationLayers);

void DestroyLogicalDevice(VkContext& ctx);

void CreateSurface(VkContext& ctx, GLFWwindow* window);

void DestroySurface(VkContext& ctx);

std::optional<uint32_t> FindPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface);

void CreateSwapChain(VkContext& ctx);
void DestroySwapChain(VkContext& ctx);
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

void CreateImageViewForSwapChain(VkContext& ctx);

void CreateGraphicsPipeline(VkContext& ctx);

void LoadShaderModules(VkContext& ctx);

void DestroyPipelineLayout(VkContext& ctx);

void CreateRenderPasses(VkContext& ctx);

void DestroyRenderPass(VkContext& ctx);

void DestroyPipeline(VkContext& ctx);

void CreateFramebuffers(VkContext& ctx);

void DestroyFramebuffers(VkContext& ctx);

void CreateCommandPool(VkContext& ctx);

void DestroyCommandPool(VkContext& ctx);

void CreateCommandBuffer(VkContext& ctx);

void RecordCommandBuffer(VkContext& ctx, uint32_t imageIndex);

void DrawFrame(VkContext& ctx);

void CreateSyncObjects(VkContext& ctx);

void DestroySyncObjects(VkContext& ctx);

void RecreateSwapChain(VkContext& ctx);

void CreateVertexBuffer(VkContext& ctx);

void DestroyVertexBuffer(VkContext& ctx);

void CreateIndexBuffer(VkContext& ctx);
void DestroyIndexBuffer(VkContext& ctx);
/// <summary>
/// Custom vkbuffer factory to encapsulate the buffer creation process and
/// avoid repeating boring code
/// </summary>
/// <param name="size"></param>
/// <param name="usage"></param>
/// <param name="properties"></param>
/// <param name="buffer"></param>
/// <param name="bufferMemory"></param>
/// <param name="device"></param>
void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties, VkBuffer& buffer, 
    VkDeviceMemory& bufferMemory, VkContext& ctx);

void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkContext& ctx);

void CreateDescriptorSetLayout(VkContext& ctx);

void DestroyDescriptorSets(VkContext& ctx);

void CreateUniformBuffers(VkContext& vkContext);

void UpdateUniformBuffer(VkContext& vkContext, uint32_t currentImage);

void CreateDescriptorPool(VkContext& ctx);

void CreateDescriptorSets(VkContext& ctx);