#pragma once

#include "renderer/vulkan/Base.hpp"
#include "renderer/vulkan/Device.hpp"

/*
 *  Command pool and command buffer related functions
 */

namespace vk
{
    VkResult beginCommand(const VkCommandBuffer &commandBuffer);
    void submitCommand(const Device &device, const VkCommandBuffer &commandBuffer, const VkQueue &queue);
    VkResult createCommandPool(const Device &device, uint32_t queueFamilyIndex, VkCommandPool *commandPool);
    VkCommandBuffer createCommandBuffer(const Device &device, const VkCommandPool &commandPool);
    VkResult createCommandBuffers(const Device &device, const VkCommandPool &commandPool, std::vector<VkCommandBuffer> &commandBuffers, size_t cmdBuffCount);
    void freeCommandBuffers(const Device &device, const VkCommandPool &commandPool, std::vector<VkCommandBuffer> &commandBuffers);
}
