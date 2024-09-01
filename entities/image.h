#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <map>
#include <string>
#include "io/image-load.h"
struct VkContext;
namespace entities {
    struct Image {
        VkImage mImage;
        VkImageView mImageView;
        const std::string mName;
    };

    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
        VkImageLayout newLayout, VkCommandPool commandPool, VkDevice device,
        VkQueue graphicsQueue);

    /// <summary>
    /// Storage for images that will be used as 2d rgba textures. They reside on the GPU, are 
    /// not accessible by the CPU, are copy destinations and can be used by samplers. Other
    /// kinds of textures, like depth, need their own texture managers because each kind of
    /// image has it's own VkDeviceMemory (VkDeviceMemory is a scarce resource that has to
    /// be saved).
    /// </summary>
    class GpuTextureManager {
    public:
        GpuTextureManager(VkContext* ctx, std::vector<io::ImageData*> images);
        ~GpuTextureManager();
        VkImage GetImage(const std::string& name)const;
        VkImageView GetImageView(const std::string& name)const;
        const VkContext* mCtx;
    private:
        VkDeviceMemory mDeviceMemory;
        std::map<std::string, Image> mImageTable;
    };
    /// <summary>
    /// Stores the render to texture targets.
    /// TODO resize: the render to texture targets may need to be destroyed then recreated when the screen size changes.
    /// </summary>
    class RenderToTextureTargetManager {
    public:
        struct RenderToTextureImageCreateData {
            uint32_t w, h;
            VkFormat format;
            VkImageUsageFlags usage;
            std::string name;
        };
        RenderToTextureTargetManager(VkContext* ctx, std::vector<RenderToTextureImageCreateData> imgs);
        const VkContext* mCtx;
    private:
        VkDeviceMemory mDeviceMemory = VK_NULL_HANDLE;
        std::map<std::string, Image> mImageTable;
    };

    class DepthBufferManager {
    public:
        static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
        static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
        struct DepthBufferCreationData {
            uint32_t w;
            uint32_t h;
            std::string name;
        };
        DepthBufferManager(VkContext* ctx, std::vector<DepthBufferCreationData> images);
        ~DepthBufferManager();
        VkImageView GetImageView(const std::string& name)const {
            return mImageTable.at(name).mImageView;
        }
        const VkContext* mCtx;
    private:
        VkDeviceMemory mDeviceMemory;
        std::map<std::string, Image> mImageTable;
    };
}