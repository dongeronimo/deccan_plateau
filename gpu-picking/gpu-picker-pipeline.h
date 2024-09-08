#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
struct VkContext;
struct CameraUniformBuffer;
namespace entities {
    class Renderable;
}
namespace GpuPicker {
    const std::string GPU_PICKER_RENDER_PASS_TARGET = "gpuPickerRenderPassTargetImage";
    class GpuPickerPipeline {
    public:
        GpuPickerPipeline(VkContext* ctx,
            VkRenderPass renderPass,
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
            const std::string& name
        );
        const std::string mName;
        const VkContext* mCtx;
        const VkRenderPass mRenderPass;
        VkPipeline GetPipeline()const { return pipeline; }
        VkPipelineLayout GetPipelineLayout()const { return pipelineLayout; }
        void Bind(VkCommandBuffer cmd);
        void DrawRenderable(entities::Renderable* obj, CameraUniformBuffer* camera,
            VkCommandBuffer cmd);

        void ScheduleTransferImageFromGPUtoCPU(VkCommandBuffer cmd,
            VkImage gpuImage, uint32_t w, uint32_t h);
        std::vector<uint8_t> GetImage();
    private:


        VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    };
}