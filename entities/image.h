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
}