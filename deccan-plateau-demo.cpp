#include "deccan-plateau-demo.h"
#include "my-vk.h"
#include "my-vk-extensions.h"
#include "my-vk-validationLayers.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
VkContext vkContext{};

const char* VkSystemAllocationScopeToString(VkSystemAllocationScope s) {
    switch (s) {
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            return "VK_SYSTEM_ALLOCATION_SCOPE_COMMAND";
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            return "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT";
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_CACHE";
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_DEVICE";
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE";
        default:
            return "INVALID VK SYSTEM ALLOCATION SCOPE";
    }
}
void* myAllocationFunction(
    void* pUserData,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope allocationScope) {
    //TODO: Use some cool custom allocator
    void* result = _aligned_malloc(size, alignment);//visual studio lacks std::aligned_alloc 
#ifdef PRINT_ALLOCATIONS 
    printf("allocated %zu bytes with %zu alignment @%p for scope %s\n", size, alignment, result, 
        VkSystemAllocationScopeToString( allocationScope ));
#endif
    return result;
}

void* myReallocationFunction(
    void* pUserData,
    void* pOriginal,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope allocationScope) {
    // Implement custom reallocation logic
    void* result = _aligned_realloc(pOriginal, size, alignment);
    //TODO: Use some cool custom allocator
#ifdef PRINT_ALLOCATIONS
    printf("reallocated %zu bytes with %zu alignment from @%p to @%p for scope %d\n",
        size, alignment, pOriginal, result, VkSystemAllocationScope(allocationScope));
#endif
    return result;
}

void myFreeFunction(
    void* pUserData,
    void* pMemory) {
    //TODO: Use some cool custom allocator
#ifdef PRINT_ALLOCATIONS 
    printf("deleting @%p\n", pMemory);
#endif
    _aligned_free(pMemory);
}


int main(int argc, char** argv)
{
    //glfw initialization, for window system. I could have used a win32 window but it would
    //be much more work
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//don't want to use opengl
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulkan", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* wnd, int w, int h) {
        vkContext.framebufferResized = true;
    });
    //prints the available extensions
    printf("Extensions:\n");
    auto extensions = GetExtensions();
    for (auto& ext : extensions)
    {
        printf("  %s\n", ext.extensionName);
    }
    ///Vulkan initialization
    vkContext.window = window;
    vkContext.clearColor = { 1.0f, 0.0f, 1.0f, 1.0f };
    vkContext.customAllocators.myAllocationFunction = myAllocationFunction;
    vkContext.customAllocators.myReallocationFunction = myReallocationFunction;
    vkContext.customAllocators.myFreeFunction = myFreeFunction;
    CreateVkInstance(vkContext.instance, vkContext.customAllocators);
    SetupDebugMessenger(vkContext.instance, vkContext.debugMessenger, vkContext.customAllocators);
    CreateSurface(vkContext, window);
    SelectPhysicalDevice(vkContext);
    CreateLogicalQueue(vkContext, EnableValidationLayers(), GetValidationLayerNames());
    CreateSwapChain(vkContext);
    CreateImageViewForSwapChain(vkContext);
    CreateRenderPasses(vkContext);
    CreateDescriptorSetLayout(vkContext);
    CreateGraphicsPipeline(vkContext);
    CreateFramebuffers(vkContext);
    CreateCommandPool(vkContext);
    CreateVertexBuffer(vkContext);
    CreateIndexBuffer(vkContext);
    CreateUniformBuffers(vkContext);
    CreateDescriptorPool(vkContext);
    CreateDescriptorSets(vkContext);
    CreateCommandBuffer(vkContext);
    CreateSyncObjects(vkContext);

    MainLoop(window);

    glfwDestroyWindow(window);

    //cleanup
    vkDeviceWaitIdle(vkContext.device);
    DestroyDescriptorSets(vkContext);
    DestroyPipeline(vkContext);
    DestroyPipelineLayout(vkContext);
    DestroyFramebuffers(vkContext);
    DestroyRenderPass(vkContext);
    DestroySwapChain(vkContext);
    DestroyImageViews(vkContext);
    DestroySyncObjects(vkContext);
    DestroyCommandPool(vkContext);
    DestroyVertexBuffer(vkContext);
    DestroyIndexBuffer(vkContext);
    DestroyLogicalDevice(vkContext);
    DestroySurface(vkContext);
    DestroyDebugMessenger(vkContext.instance, vkContext.debugMessenger, vkContext.customAllocators);
    DestroyVkInstance(vkContext.instance, vkContext.customAllocators);
    glfwTerminate();
    return 0;
}

void MainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        DrawFrame(vkContext);
    }
}
