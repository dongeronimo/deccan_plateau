#include "image.h"
#include "my-vk.h"
#include <stdexcept>
#include "object_namer.h"
#include "concatenate.h"
#include "commandBufferUtils.h"
void SetImageObjName(VkImage img, const std::string& baseName) {
    auto name = Concatenate(baseName, "ImageObject");
    SET_NAME(img, VK_OBJECT_TYPE_IMAGE, name.c_str());
}

uint32_t FindMemoryTypesCompatibleWithAllImages(const std::vector<VkMemoryRequirements>& memoryRequirements) {
    uint32_t compatibleMemoryTypes = ~0; // Start with all bits set to 1
    for (const auto& memreq : memoryRequirements) {
        // Intersect with current memoryTypeBits
        compatibleMemoryTypes &= memreq.memoryTypeBits;
    }
    return compatibleMemoryTypes;
}
VkImage CreateImage(VkDevice device, uint32_t w, uint32_t h, VkFormat format,
    VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = w;
    imageInfo.extent.height = h;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;//VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;//VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkImage image;
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    return image;
}
uint32_t __findMemoryType(uint32_t typeFilter, 
    VkMemoryPropertyFlags properties, 
    VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
    VkCommandPool commandPool, VkDevice device, VkQueue graphicsQueue)
{
    VkCommandBuffer commandBuffer = CreateCommandBuffer(commandPool, device, "copy buffer to image");
    BeginRecordingCommands(commandBuffer);
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    SubmitAndFinishCommands(commandBuffer, graphicsQueue, device, commandPool);
}

namespace entities {
    GpuTextureManager::GpuTextureManager(VkContext* ctx,
        std::vector<io::ImageData*> images):mCtx(ctx)
    {
        //Calculate the memory requirements
        std::vector<VkImage> vkImages;
        std::vector<VkMemoryRequirements> memoryRequirements;
        VkDeviceSize totalSize = 0;
        VkDeviceSize currentOffset = 0;
        for (int i = 0; i < images.size(); i++) {
            VkImage image = CreateImage(ctx->device, images[i]->w, images[i]->h,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            SetImageObjName(image, images[i]->name);
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(ctx->device, image, &memRequirements);
            vkImages.push_back(image);
            memoryRequirements.push_back(memRequirements);
            // Align the current offset to the required alignment of this image
            currentOffset = (currentOffset + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);
            // Update the total size needed
            totalSize = currentOffset + memRequirements.size;
            // Move the offset forward
            currentOffset += memRequirements.size;
        }
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);
        //find the memory type that is compatible with all images        
        uint32_t compatibleMemoryTypes = FindMemoryTypesCompatibleWithAllImages(memoryRequirements);
        uint32_t memoryTypeIndex = __findMemoryType(compatibleMemoryTypes, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ctx->physicalDevice);
        //allocate the big memory for all the images
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = totalSize;  // Total size from previous calculations
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        if (vkAllocateMemory(ctx->device, &allocInfo, nullptr, &mDeviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate large device memory!");
        }
        SET_NAME(mDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "GpuTexturesDeviceMemory");
        //now we have to bind the vkImages to sections of the vkDeviceMemory
        currentOffset = 0;
        for (int i = 0; i < vkImages.size(); i++) {
            VkMemoryRequirements memRequirements = memoryRequirements[i];
            // Align the current offset to the required alignment of this image
            currentOffset = (currentOffset + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);
            // Bind the image to the memory at the aligned offset
            if (vkBindImageMemory(ctx->device, vkImages[i], mDeviceMemory, currentOffset) != VK_SUCCESS) {
                throw std::runtime_error("Failed to bind image memory!");
            }
            //copy to the staging buffer
            VkBuffer textureStagingBuffer;
            VkDeviceMemory textureStagingMemory;
            CreateBuffer(images[i]->size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //will be a source of transfer 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,//memory visible on cpu side 
                textureStagingBuffer, textureStagingMemory, *ctx);
            void* textureDataPtr;
            vkMapMemory(ctx->device, textureStagingMemory, 0, images[i]->size, 0, &textureDataPtr);
            memcpy(textureDataPtr, images[i]->pixels.data(), static_cast<size_t>(images[i]->size));
            vkUnmapMemory(ctx->device, textureStagingMemory);
            auto __name = Concatenate(images[i]->name, "StagingBuffer");
            SET_NAME(textureStagingBuffer, VK_OBJECT_TYPE_BUFFER, __name.c_str());
            __name = Concatenate(images[i]->name, "StagingBufferDeviceMemory");
            SET_NAME(textureStagingMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, __name.c_str());

            //change the image from whatever it is when it's created to a destination of a copy with optimal layout
            TransitionImageLayout(vkImages[i], VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                ctx->commandPool, ctx->device, ctx->graphicsQueue);
            //now that the image can be the dest of a copy, do it
            CopyBufferToImage(textureStagingBuffer, vkImages[i],
                static_cast<uint32_t>(images[i]->w), static_cast<uint32_t>(images[i]->h),
                ctx->commandPool, ctx->device, ctx->graphicsQueue);
            //transition it from destination of copy to shader-only.
            TransitionImageLayout(vkImages[i], VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                ctx->commandPool, ctx->device, ctx->graphicsQueue);


            //create the view
            VkImageViewCreateInfo textureViewInfo{};
            textureViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            textureViewInfo.image = vkImages[i];
            textureViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            textureViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            textureViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            textureViewInfo.subresourceRange.baseMipLevel = 0;
            textureViewInfo.subresourceRange.levelCount = 1;
            textureViewInfo.subresourceRange.baseArrayLayer = 0;
            textureViewInfo.subresourceRange.layerCount = 1;
            VkImageView textureImageView;
            if (vkCreateImageView(ctx->device, &textureViewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }
            __name = Concatenate(images[i]->name, "ImageView");
            SET_NAME(textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW, __name.c_str());
            vkDestroyBuffer(ctx->device, textureStagingBuffer, nullptr);
            vkFreeMemory(ctx->device, textureStagingMemory, nullptr);
            //add to the table
            Image img{
                vkImages[i],
                textureImageView,
                images[i]->name
            };
            mImageTable.insert({ images[i]->name, img });
            // Move the offset forward
            currentOffset += memRequirements.size;
        }
        
    }
    GpuTextureManager::~GpuTextureManager()
    {
        for (auto& kv : mImageTable) {
            vkDestroyImage(mCtx->device, kv.second.mImage, nullptr);
            vkDestroyImageView(mCtx->device, kv.second.mImageView, nullptr);
        }
        vkFreeMemory(mCtx->device, mDeviceMemory, nullptr);
        mImageTable.clear();
    }
    VkImage GpuTextureManager::GetImage(const std::string& name) const
    {
        return mImageTable.at(name).mImage;
    }
    VkImageView GpuTextureManager::GetImageView(const std::string& name) const
    {
        return mImageTable.at(name).mImageView;
    }
    void TransitionImageLayout(VkImage image, VkFormat format, 
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkCommandPool commandPool, VkDevice device, VkQueue graphicsQueue)
    {
        VkCommandBuffer commandBuffer = CreateCommandBuffer(commandPool, device, "image transition command");//beginSingleTimeCommands(commandPool, device);
        BeginRecordingCommands(commandBuffer);
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
        SubmitAndFinishCommands(commandBuffer, graphicsQueue, device, commandPool);
    }

    VkFormat DepthBufferManager::findDepthFormat(VkPhysicalDevice physicalDevice)
    {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            physicalDevice
        );
    }

    VkFormat DepthBufferManager::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice)
    {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }

    DepthBufferManager::DepthBufferManager(VkContext* ctx,
        std::vector<DepthBufferCreationData> images)
        :mCtx(ctx), mDeviceMemory(VK_NULL_HANDLE)
    {
        //Calculate the memory requirements
        std::vector<VkImage> vkImages;
        std::vector<VkMemoryRequirements> memoryRequirements;
        VkDeviceSize totalSize = 0;
        VkDeviceSize currentOffset = 0;
        for (int i = 0; i < images.size(); i++) {
            VkFormat depthFormat = findDepthFormat(ctx->physicalDevice);
            VkImage image = CreateImage(ctx->device, images[i].w, images[i].h,
                depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            SetImageObjName(image, images[i].name);
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(ctx->device, image, &memRequirements);
            vkImages.push_back(image);
            memoryRequirements.push_back(memRequirements);
            // Align the current offset to the required alignment of this image
            currentOffset = (currentOffset + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);
            // Update the total size needed
            totalSize = currentOffset + memRequirements.size;
            // Move the offset forward
            currentOffset += memRequirements.size;
        }
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);
        uint32_t compatibleMemoryTypes = FindMemoryTypesCompatibleWithAllImages(memoryRequirements);
        uint32_t memoryTypeIndex = __findMemoryType(compatibleMemoryTypes,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ctx->physicalDevice);
        //allocate the big memory for all the images
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = totalSize;  // Total size from previous calculations
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        if (vkAllocateMemory(ctx->device, &allocInfo, nullptr, &mDeviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate large device memory!");
        }
        SET_NAME(mDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "DepthBuffersDeviceMemory");
        //now we have to bind the vkImages to sections of the vkDeviceMemory
        currentOffset = 0;
        for (int i = 0; i < vkImages.size(); i++) {
            VkMemoryRequirements memRequirements = memoryRequirements[i];
            // Align the current offset to the required alignment of this image
            currentOffset = (currentOffset + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);
            // Bind the image to the memory at the aligned offset
            if (vkBindImageMemory(ctx->device, vkImages[i], mDeviceMemory, currentOffset) != VK_SUCCESS) {
                throw std::runtime_error("Failed to bind image memory!");
            }
            //create the view
            VkImageViewCreateInfo depthImageViewInfo{};
            depthImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthImageViewInfo.image = vkImages[i];
            depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthImageViewInfo.format = findDepthFormat(ctx->physicalDevice);
            depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthImageViewInfo.subresourceRange.baseMipLevel = 0;
            depthImageViewInfo.subresourceRange.levelCount = 1;
            depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
            depthImageViewInfo.subresourceRange.layerCount = 1;
            VkImageView depthImageView;
            if (vkCreateImageView(ctx->device, &depthImageViewInfo,
                nullptr, &depthImageView) != VK_SUCCESS) {
                throw std::runtime_error("failed to create depth image view!");
            }
            auto __name = Concatenate(images[i].name, "ImageView");
            SET_NAME(depthImageView, VK_OBJECT_TYPE_IMAGE_VIEW, __name.c_str());
            //add to the table
            Image img{
                vkImages[i],
                depthImageView,
                images[i].name
            };
            mImageTable.insert({ images[i].name, img });
            // Move the offset forward
            currentOffset += memRequirements.size;
        }
    }
    DepthBufferManager::~DepthBufferManager()
    {
        for (auto& kv : mImageTable) {
            vkDestroyImage(mCtx->device, kv.second.mImage, nullptr);
            vkDestroyImageView(mCtx->device, kv.second.mImageView, nullptr);
        }
        vkFreeMemory(mCtx->device, mDeviceMemory, nullptr);
    }
}