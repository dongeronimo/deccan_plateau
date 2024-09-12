#include "gpu-picker-pipeline.h"
#include "entities/pipeline.h"
#include "vk/my-vk.h"
#include <stdexcept>
#include <utils/concatenate.h>
#include <utils/object_namer.h>
#include "entities/renderable.h"
#include "entities/mesh.h"
#include "vk\my-device.h"
static VkBuffer buffer = VK_NULL_HANDLE;
static VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
VkDeviceSize bufferSize;
namespace GpuPicker {
    GpuPickerPipeline::GpuPickerPipeline(VkContext* ctx, 
        VkRenderPass renderPass, 
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts, 
        const std::string& name)
        :mCtx(ctx), mRenderPass(renderPass), descriptorSetLayouts(descriptorSetLayouts),
        mName(name), pipeline(VK_NULL_HANDLE)
    {
        assert(renderPass != VK_NULL_HANDLE);//did you forget it?
        assert(descriptorSetLayouts.size() > 0); 

        using namespace entities;
        //load the shader modules
        vertexShaderModule = Pipeline::LoadShaderModule(myvk::Device::gDevice->GetDevice(), 
            "gpu_picker_vert.spv");
        fragmentShaderModule = Pipeline::LoadShaderModule(myvk::Device::gDevice->GetDevice(), 
            "gpu_picker_frag.spv");
        //description of the shader stages
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = Pipeline::CreateShaderStageInfoForVertexAndFragment(
            vertexShaderModule, fragmentShaderModule);
        //the dynamic states - they will have to set up in draw time
        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        //vertex input description - the same of hello pipeline bc we are rendering
        //meshes like it
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
        
        // The push constant
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;  // Specify the shader stage(s)
        pushConstantRange.offset = 0;                               // Offset within the push constant block
        pushConstantRange.size = sizeof(int); //The id is just an int
        
        //pipeline layout, to pass data to the shaders, sends nothing for now
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        //we use no push constants for now
        pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional
        if (vkCreatePipelineLayout(myvk::Device::gDevice->GetDevice(), &pipelineLayoutInfo,
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
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
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
        if (vkCreateGraphicsPipelines(myvk::Device::gDevice->GetDevice(), VK_NULL_HANDLE,
            1,//number of pipelines 
            &pipelineInfo, //list of pipelines (only one) 
            nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        SET_NAME(pipeline, VK_OBJECT_TYPE_PIPELINE, name.c_str());
        //the shader modules are no longer necessary by now.
        vkDestroyShaderModule(myvk::Device::gDevice->GetDevice(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(myvk::Device::gDevice->GetDevice(), fragmentShaderModule, nullptr);
    }

    GpuPickerPipeline::~GpuPickerPipeline()
    {
        vkDestroyPipeline(myvk::Device::gDevice->GetDevice(), pipeline, nullptr);
        //for (auto dsl : descriptorSetLayouts) {
        //    vkDestroyDescriptorSetLayout(myvk::Device::gDevice->GetDevice(), dsl, nullptr);
        //}
        vkDestroyPipelineLayout(myvk::Device::gDevice->GetDevice(), pipelineLayout, nullptr);
        vkFreeMemory(myvk::Device::gDevice->GetDevice(), bufferMemory, nullptr);
        vkDestroyBuffer(myvk::Device::gDevice->GetDevice(), buffer, nullptr);
    }

    void GpuPickerPipeline::Bind(VkCommandBuffer cmd)
    {
        vkCmdBindPipeline(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void GpuPickerPipeline::DrawRenderable(entities::Renderable* go, CameraUniformBuffer* camera, VkCommandBuffer cmdBuffer)
    {
        SetMark({ 1.0f, 0.8f, 1.0f, 1.0f }, go->mName, cmdBuffer, *mCtx);
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
        //push the id to the shader using the push constant.
        int id = go->mId;
        vkCmdPushConstants(
            cmdBuffer,                   // Command buffer
            pipelineLayout,                  // Pipeline layout
            VK_SHADER_STAGE_FRAGMENT_BIT,      // Shader stage(s)
            0,                               // Offset within the push constant block
            sizeof(int),                     // Size of the push constant data
            &id // Pointer to the data
        );
        //Draw command
        vkCmdDrawIndexed(cmdBuffer,
            static_cast<uint32_t>(go->mMesh->NumberOfIndices()),
            1,
            0,
            0,
            0);
        static PFN_vkCmdDebugMarkerEndEXT __vkCmdDebugMarkerEndEXT;
        if (__vkCmdDebugMarkerEndEXT == VK_NULL_HANDLE) {
            __vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(myvk::Device::gDevice->GetDevice(), "vkCmdDebugMarkerEndEXT");
        }
        __vkCmdDebugMarkerEndEXT(cmdBuffer);
    }

    void GpuPickerPipeline::ScheduleTransferImageFromGPUtoCPU(VkCommandBuffer cmd,
        VkImage gpuImage, uint32_t w, uint32_t h)
    {
        //memory barrier to wait for the image to be ready and once
        //ready transition it from it's original layout to the transfer
        //source layout
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;  // Starting layout
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;  // Target layout
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = gpuImage;  // Your image handle
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // If the image contains color data
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        // Define the source and destination access masks for the transfer
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;  // Previous access for GENERAL layout
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;  // Prepare for transfer read
        // Submit the pipeline barrier
        vkCmdPipelineBarrier(
            cmd,                 // Command buffer recording
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // Source stage (depends on previous usage)
            VK_PIPELINE_STAGE_TRANSFER_BIT,      // Destination stage (for transfer)
            0,                               // No dependency flags
            0, nullptr,                      // No memory barriers
            0, nullptr,                      // No buffer memory barriers
            1, &barrier                      // Image memory barrier
        );
        //create the buffer and it's memory
        bufferSize = w * h * 4;  // Size of the buffer in bytes
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;  // Size of the buffer
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // Buffer will be used as a transfer destination
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // Only one queue will use this buffer

        if (buffer == VK_NULL_HANDLE) {
            if (vkCreateBuffer(myvk::Device::gDevice->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create buffer!");
            }
            SET_NAME(buffer, VK_OBJECT_TYPE_BUFFER, "gpuPickerBuffer");
        }
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(myvk::Device::gDevice->GetDevice(), buffer, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, *mCtx);
        if (bufferMemory == VK_NULL_HANDLE) {
            if (vkAllocateMemory(myvk::Device::gDevice->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate buffer memory!");
            }
            SET_NAME(bufferMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "gpuPickerBufferMemory");
            vkBindBufferMemory(myvk::Device::gDevice->GetDevice(), buffer, bufferMemory, 0);
        }

        //copy the content from the image to the buffer
        VkBufferImageCopy region{};
        region.bufferOffset = 0;  // Start at the beginning of the buffer
        region.bufferRowLength = 0;  // Tightly packed (0 means the buffer has no padding)
        region.bufferImageHeight = 0;  // Tightly packed (0 means no padding)
        // Specify the subresource layers of the image
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Assuming this is a color image
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        // Specify the region of the image to copy
        region.imageOffset = { 0, 0, 0 };  // Starting point (x, y, z) in the image
        region.imageExtent = {
            w,   // Width of the image
            h,  // Height of the image
            1        // Depth (for 2D images)
        };
        // Record the command to copy the image to the buffer
        vkCmdCopyImageToBuffer(
            cmd,               // Command buffer
            gpuImage,                    // Source image (the image to copy from)
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // Layout of the source image
            buffer,                   // Destination buffer (the buffer to copy to)
            1,                           // Number of regions to copy
            &region                      // The region information
        );
        //the image isn't available here yet. It'll only be available after the queue is submitted
        // 
        // 
        ////Copies to the vector
        //std::vector<uint8_t> pixels(bufferSize);
        //void* bufferAddr = nullptr;
        //vkMapMemory(myvk::Device::gDevice->GetDevice(), bufferMemory, 0, bufferSize, 0, &bufferAddr);
        //memcpy(pixels.data(), bufferAddr, bufferSize);

    }

    std::vector<uint8_t> GpuPickerPipeline::GetImage()
    {
        std::vector<uint8_t> pixels(bufferSize, 0);
        void* bufferAddr = nullptr;
        vkMapMemory(myvk::Device::gDevice->GetDevice(), bufferMemory, 0, bufferSize, 0, &bufferAddr);
        memcpy(pixels.data(), bufferAddr, bufferSize);
        vkUnmapMemory(myvk::Device::gDevice->GetDevice(), bufferMemory);
        return pixels;
    }

}