#pragma once

#include "renderer/vulkan/Base.hpp"
#include "renderer/vulkan/Device.hpp"

/*
 *  Command pool and command buffer related functions
 */

namespace vk
{
    VkCommandBuffer beginOneTimeCommand(const Device &device, const VkCommandPool &commandPool);
    void endOneTimeCommand(const Device &device, const VkCommandBuffer &commandBuffer, const VkCommandPool &commandPool, const VkQueue &graphicsQueue);
    VkResult createCommandPool(const Device &device, VkCommandPool *commandPool);
    VkResult createCommandBuffers(const Device &device, const VkCommandPool &commandPool, std::vector<VkCommandBuffer> &commandBuffers, size_t cmdBuffCount);
    void freeCommandBuffers(const Device &device, const VkCommandPool &commandPool, std::vector<VkCommandBuffer> &commandBuffers);
}
