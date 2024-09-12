#include "commandBufferUtils.h"
#include "utils/object_namer.h"
#include <cassert>
VkCommandBuffer CreateCommandBuffer(VkCommandPool commandPool,
    VkDevice device,
    const std::string& name) {
    assert(commandPool != VK_NULL_HANDLE);
    //a temporary command buffer for copy
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    SET_NAME(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, name.c_str());
    return commandBuffer;
}
void BeginRecordingCommands(VkCommandBuffer cmd) {
    //begin recording commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //this command buffer will be used only once
    vkBeginCommandBuffer(cmd, &beginInfo);
}
void CopyBuffer(VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size,
    VkCommandBuffer cmd, VkBuffer src, VkBuffer dst) {
    //copy command
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset; // Optional
    copyRegion.dstOffset = dstOffset; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);

}

void SubmitAndFinishCommands(VkCommandBuffer cmd, VkQueue queue, VkDevice device,
    VkCommandPool commandPool) {
    //end the recording
    vkEndCommandBuffer(cmd);
    //submit the command
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(queue,
        1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);//cpu waits until copy is done
    //free the command buffer
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}