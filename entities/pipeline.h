#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
struct VkContext;
namespace entities {
    class Pipeline {
    public:
        /// <summary>
        /// Utility function to load a shader module.
        /// </summary>
        /// <param name="device"></param>
        /// <param name="name"></param>
        /// <returns></returns>
        static VkShaderModule LoadShaderModule(VkDevice device, const std::string& name);
        Pipeline(VkContext* ctx,
            VkRenderPass renderPass,
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
            const std::string& name
        );
        const std::string mName;
        const VkContext* mCtx;
        const VkRenderPass mRenderPass;
        VkPipeline GetPipeline()const { return pipeline; }
        VkPipelineLayout GetPipelineLayout()const { return pipelineLayout; }
    private:
        VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    };
}