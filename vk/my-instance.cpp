#include "my-instance.h"
#include "vk/my-vk-validationLayers.h"
#include "vk/my-vk-extensions.h"
#include <stdexcept>
#include <cassert>
namespace myvk {
    Instance* Instance::gInstance;
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
    Instance::Instance(GLFWwindow* window)
        :mWindow(window)
    {
        assert(Instance::gInstance == nullptr);//i'll have only one intance, created at the beginning
        Instance::gInstance = this;
        CreateInstance();
        SetupDebugMessenger(mInstance, mDebugMessager);
        glfwCreateWindowSurface(mInstance, window, nullptr, &mSurface);
        //how many devices?
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
        if (deviceCount == 0)
            throw std::runtime_error("no gpu that suports vk, bye");
        //Get them
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());
        for (auto& device : devices) {
            VkPhysicalDeviceProperties properties = {};
            vkGetPhysicalDeviceProperties(device, &properties);
            VkPhysicalDeviceFeatures features = {};
            vkGetPhysicalDeviceFeatures(device, &features);
            PhysicalDeviceProperties props(properties, features, device);
            mPhysicalDevices.push_back(props);
        }
    }
    Instance::~Instance()
    {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        DestroyDebugMessenger(mInstance, mDebugMessager);
        vkDestroyInstance(mInstance, nullptr);
    }
    VkPhysicalDevice Instance::GetPhysicalDevice() const
    {
        assert(mChosenDeviceId != UINT32_MAX); //choose the device before calling this.
        return mPhysicalDevices[mChosenDeviceId].mDevice;
    }
    void Instance::ChoosePhysicalDevice(VkPhysicalDeviceType type, PhysicalDeviceFeatureQueryParam samplerAnisotropy)
    {
        for (int i = 0; i < mPhysicalDevices.size(); i++) {
            auto& d = mPhysicalDevices[i];
            if (d.mProperties.deviceType == type &&
                samplerAnisotropy == DONT_CARE || (samplerAnisotropy == YES && d.mFeatures.samplerAnisotropy == 1)
                ){
                mChosenDeviceId = i;
            }
        }
        if (mChosenDeviceId == UINT32_MAX)
            throw new std::runtime_error("No physical device satisfies all criteria");
    }
    void Instance::CreateInstance()
    {
        ///Creates the instance
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
        if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
            throw std::runtime_error("fail to create vk instance");
        }
    }
}
