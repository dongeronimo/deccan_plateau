#include "pipeline.h"
#include <io/asset-paths.h>
#include <fstream>
#include "concatenate.h"
#include "my-vk.h"
#include "object_namer.h"
#include "renderable.h"
#include "mesh.h"
namespace entities {
    VkShaderModule Pipeline::LoadShaderModule(VkDevice device, const std::string& name)
    {
        auto path = io::CalculatePathForShader(name);
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("failed to open file!");
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> binaryCode(fileSize);
        file.seekg(0);
        file.read(binaryCode.data(), fileSize);
        file.close();
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = binaryCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(binaryCode.data());
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            auto errMsg = Concatenate("failed to create shader module: ", name);
            throw std::runtime_error(errMsg);
        }
        SET_NAME(shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, name.c_str());
        return shaderModule;
    }
    Pipeline::Pipeline(VkContext* ctx,
        VkRenderPass renderPass,
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
        const std::string& name):
        mCtx(ctx),mRenderPass(renderPass),mName(name), descriptorSetLayouts(descriptorSetLayouts)
    {
        //load the shader modules
        vertexShaderModule = LoadShaderModule(ctx->device, "hello_shader_vert.spv");
        fragmentShaderModule = LoadShaderModule(ctx->device, "hello_shader_frag.spv");
        //description of the shader stages
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = CreateShaderStageInfoForVertexAndFragment(
            vertexShaderModule, fragmentShaderModule);
        //the dynamic states - they will have to set up in draw time
        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        //vertex input description
        auto bindingDescription = GetBindingDescription();
        auto attributeDescriptions = GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        //input description: the geometry input will be triangles
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        //since the viewport and scissors are dynamic, is just need to tell the pipeline their number
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        //rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; //only relevant when doing shadow maps
        rasterizer.rasterizerDiscardEnable = VK_FALSE; //if true the geometry will never pass thru the rasterizer stage
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0f;
        //multisampling TODO: Disabled for now
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        //color blend
        //  per framebuffer
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; //TODO: no color blending for now
        //  global
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE; //TODO: no color blending for now
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        //pipeline layout, to pass data to the shaders, sends nothing for now
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        //we use no push constants for now
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo,
            nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
        auto plname = Concatenate(name, "PipelineLayout");
        SET_NAME(pipelineLayout,
            VK_OBJECT_TYPE_PIPELINE_LAYOUT, plname.c_str());

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional
        //the actual pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();    //add the shader stages
        //set the states
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        //link the pipeline layout to the pipeline
        pipelineInfo.layout = pipelineLayout;
        //link the render pass to the pipeline
        pipelineInfo.renderPass = mRenderPass;
        pipelineInfo.subpass = 0;
        if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE,
            1,//number of pipelines 
            &pipelineInfo, //list of pipelines (only one) 
            nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        SET_NAME(pipeline, VK_OBJECT_TYPE_PIPELINE, name.c_str());
        //the shader modules are no longer necessary by now.
        vkDestroyShaderModule(ctx->device, vertexShaderModule, nullptr);
        vkDestroyShaderModule(ctx->device, fragmentShaderModule, nullptr);
    }
    std::vector<VkPipelineShaderStageCreateInfo> Pipeline::CreateShaderStageInfoForVertexAndFragment(VkShaderModule vs, VkShaderModule fs)
    {
        //description of the shader stages
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vs;
        vertShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fs;
        fragShaderStageInfo.pName = "main";
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { 
            vertShaderStageInfo, 
            fragShaderStageInfo };
        return shaderStages;
    }
    Pipeline::~Pipeline()
    {
        vkDestroyPipeline(mCtx->device, pipeline, nullptr);
        vkDestroyPipelineLayout(mCtx->device, pipelineLayout, nullptr);
 
    }
    void Pipeline::Bind(VkCommandBuffer cmd)
    {
        vkCmdBindPipeline(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }
    void Pipeline::DrawRenderable(Renderable* go,
        CameraUniformBuffer* camera,
        VkCommandBuffer cmdBuffer)
    {
        SetMark({ 1.0f, 0.0f, 0.0f, 1.0f }, go->mName, cmdBuffer, *mCtx);
        //copies camera data to gpu
        memcpy(mCtx->helloCameraUniformBufferAddress[mCtx->currentFrame], camera, sizeof(CameraUniformBuffer));
        //copies object data to gpu
        go->CommitDataToObjectBuffer(mCtx->currentFrame);
        go->mMesh->Bind(cmdBuffer);
        //bind the camera descriptor set
        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,   // We assume it's a graphics pipeline
            pipelineLayout,                    // The pipeline layout that matches the shader's descriptor set layouts
            0,                                 // firstSet, which is the index of the first descriptor set (set = 0)
            1,                                 // descriptorSetCount, number of descriptor sets to bind
            &mCtx->helloCameraDescriptorSets[mCtx->currentFrame],              // Pointer to the array of descriptor sets (only one in this case)
            0,                                 // dynamicOffsetCount, assuming no dynamic offsets
            nullptr                            // pDynamicOffsets, assuming no dynamic offsets
        );
        //bind the object-specific descriptor set
        // Bind the object-specific descriptor set (set = 1)
        VkDescriptorSet objectDescriptorSet = go->GetDescriptorSet(mCtx->currentFrame);
        uint32_t dynamicOffset = go->DynamicOffset(mCtx->currentFrame);
        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            1,                                 // firstSet, index of the first descriptor set (set = 1)
            1,                                 // descriptorSetCount, number of descriptor sets to bind
            &objectDescriptorSet,              // Pointer to the array of descriptor sets (only one in this case)
            1,
            &dynamicOffset
        );
        //bind the sampler
        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            2, //third set
            1,
            &mCtx->helloSamplerDescriptorSets[mCtx->currentFrame],
            0,
            nullptr
        );
        //Draw command
        vkCmdDrawIndexed(cmdBuffer,
            static_cast<uint32_t>(go->mMesh->NumberOfIndices()),
            1,
            0,
            0,
            0);
        mCtx->vkCmdDebugMarkerEndEXT(cmdBuffer);
    }
}
