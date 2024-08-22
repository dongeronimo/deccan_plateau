#include "deccan-plateau-demo.h"
#include "my-vk.h"
#include "my-vk-extensions.h"
#include "my-vk-validationLayers.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include "object_namer.h"
#include "entities/game-object.h"
#include <chrono>
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
std::vector<entities::GameObject*> gGameObjects{};

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
    vk::ObjectNamer::Instance().Init(vkContext.device);
    CreateSwapChain(vkContext);
    CreateImageViewForSwapChain(vkContext);
    CreateRenderPasses(vkContext);
    //the hello pipeline needs these descriptors.
    CreateDescriptorSetLayoutForCamera(vkContext);
    CreateDescriptorSetLayoutForObject(vkContext);
    CreateGraphicsPipeline(vkContext);
    CreateFramebuffers(vkContext);
    CreateCommandPool(vkContext);
    CreateVertexBuffer(vkContext);
    CreateIndexBuffer(vkContext);
    CreateUniformBuffersForCamera(vkContext);
    //CreateUniformBuffersForObject(vkContext);//For now it'll live here but when i have my game objects, it'll go to them
    CreateDescriptorPool(vkContext);//for now i create  both pools at the same place. In the future i'll have some kind of pool manager
    CreateDescriptorSetsForObject(vkContext);//each object will have it's own descriptor set
    CreateDescriptorSetsForCamera(vkContext);
    CreateCommandBuffer(vkContext);
    CreateSyncObjects(vkContext);

    //now that all vulkan infra is created we create the game objects
    entities::GameObject* foo = new entities::GameObject(&vkContext, "foo",
        vkContext.vertexBuffer, vkContext.indexBuffer, 6);
    foo->SetPosition(glm::vec3{ 1,0,0 });
    entities::GameObject* bar = new entities::GameObject(&vkContext, "bar",
        vkContext.vertexBuffer, vkContext.indexBuffer, 6);
    bar->SetPosition(glm::vec3{ -1,0,0 });
    gGameObjects.push_back(foo);
    gGameObjects.push_back(bar);

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
    //DestroyVertexBuffer(vkContext);
    //DestroyIndexBuffer(vkContext);
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
        //Calculate time elapsed since start and delta time
        static auto startTime = std::chrono::high_resolution_clock::now();
        static auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float secondsSinceStart = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        
        CameraUniformBuffer cameraBuffer;
        cameraBuffer.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //some perspective projection
        cameraBuffer.proj = glm::perspective(glm::radians(45.0f),
            vkContext.swapChainExtent.width / (float)vkContext.swapChainExtent.height, 0.1f, 10.0f);
        //GOTCHA: GLM is for opengl, the y coords are inverted. With this trick we the correct that
        cameraBuffer.proj[1][1] *= -1;

        uint32_t imageIndex;
        if (BeginFrame(vkContext, imageIndex)) {
            for (auto go : gGameObjects) {
                DrawGameObject(go, cameraBuffer, vkContext);
            }
            EndFrame(vkContext, imageIndex);
        }
        
    }
}
