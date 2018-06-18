#include "renderer/vulkan/CmdBuffer.hpp"
#include "Utils.hpp"

namespace vk
{
    VkCommandBuffer beginOneTimeCommand(const Device &device, const VkCommandPool &commandPool)
    {
        // todo: command pool for potential optimization
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VkResult result = vkAllocateCommandBuffers(device.logical, &allocInfo, &commandBuffer);
        LOG_MESSAGE_ASSERT(result == VK_SUCCESS, "Could not allocated command buffer: " << result);
        if (result != VK_SUCCESS) return commandBuffer;

        VkCommandBufferBeginInfo cmdInfo = {};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &cmdInfo);
        return commandBuffer;
    }

    void endOneTimeCommand(const Device &device, const VkCommandBuffer &commandBuffer, const VkCommandPool &commandPool, const VkQueue &graphicsQueue)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        vkFreeCommandBuffers(device.logical, commandPool, 1, &commandBuffer);
    }

    VkResult createCommandPool(const Device &device, VkCommandPool *commandPool)
    {
        VkCommandPoolCreateInfo cpCreateInfo = {};
        cpCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cpCreateInfo.queueFamilyIndex = device.queueFamilyIndex;
        // allow the command pool to be explicitly reset without reallocating it manually during recording each frame
        cpCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        return vkCreateCommandPool(device.logical, &cpCreateInfo, nullptr, commandPool);
    }

    VkResult createCommandBuffers(const Device &device, const VkCommandPool &commandPool, std::vector<VkCommandBuffer> &commandBuffers, size_t cmdBuffCount)
    {
        commandBuffers.resize(cmdBuffCount);

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        return vkAllocateCommandBuffers(device.logical, &allocInfo, commandBuffers.data());
    }

    void freeCommandBuffers(const Device &device, const VkCommandPool &commandPool, std::vector<VkCommandBuffer> &commandBuffers)
    {
        if (!commandBuffers.empty())
        {
            vkFreeCommandBuffers(device.logical, commandPool, (uint32_t)commandBuffers.size(), commandBuffers.data());
            commandBuffers.clear();
        }
    }
}
