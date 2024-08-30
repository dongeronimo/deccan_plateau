#pragma once
#include <vulkan/vulkan.h>
#include <string>

VkCommandBuffer CreateCommandBuffer(VkCommandPool commandPool,
    VkDevice device,
    const std::string& name);
void BeginRecordingCommands(VkCommandBuffer cmd);

void CopyBuffer(VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size,
    VkCommandBuffer cmd, VkBuffer src, VkBuffer dst);

void SubmitAndFinishCommands(VkCommandBuffer cmd, VkQueue queue, VkDevice device,
    VkCommandPool commandPool);