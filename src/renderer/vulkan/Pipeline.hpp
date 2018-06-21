#pragma once
#include "renderer/vulkan/Device.hpp"
#include "renderer/vulkan/Buffers.hpp"

/*
 * Vulkan pipeline, renderpass and shaders
 */

namespace vk
{
    struct Pipeline
    {
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPolygonMode mode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkBlendFactor blendMode = VK_BLEND_FACTOR_ZERO;
        VkBool32 depthTestEnable = VK_TRUE;
    };

    struct RenderPass
    {
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    };


    VkResult createPipeline(const Device &device, const SwapChain &swapChain, const RenderPass &renderPass, const VkDescriptorSetLayout &descriptorLayout, const VertexBufferInfo *vbInfo, Pipeline *pipeline, const char **shaders);
    void     destroyPipeline(const Device &device, Pipeline &pipeline);
    VkResult createRenderPass(const Device &device, const SwapChain &swapChain, RenderPass *renderPass);
    void     destroyRenderPass(const Device &device, RenderPass &renderPass);
}
