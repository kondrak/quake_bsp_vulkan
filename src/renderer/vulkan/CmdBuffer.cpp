#include "renderer/vulkan/CmdBuffer.hpp"
#include "Utils.hpp"

namespace vk
{
    VkResult beginCommand(const VkCommandBuffer &commandBuffer)
    {
        VkCommandBufferBeginInfo cmdInfo = {};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        return vkBeginCommandBuffer(commandBuffer, &cmdInfo);
    }

    void submitCommand(const Device &device, const VkCommandBuffer &commandBuffer, const VkQueue &queue)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkFenceCreateInfo fCreateInfo = {};
        fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence queueFence;
        vkCreateFence(device.logical, &fCreateInfo, nullptr, &queueFence);

        vkQueueSubmit(queue, 1, &submitInfo, queueFence);
 
        vkWaitForFences(device.logical, 1, &queueFence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(device.logical, queueFence, nullptr);
    }

    VkResult createCommandPool(const Device &device, uint32_t queueFamilyIndex, VkCommandPool *commandPool)
    {
        VkCommandPoolCreateInfo cpCreateInfo = {};
        cpCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cpCreateInfo.queueFamilyIndex = queueFamilyIndex;
        // allow the command pool to be explicitly reset without reallocating it manually during recording each frame
        cpCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        return vkCreateCommandPool(device.logical, &cpCreateInfo, nullptr, commandPool);
    }

    VkCommandBuffer createCommandBuffer(const Device &device, const VkCommandPool &commandPool)
    {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VK_VERIFY(vkAllocateCommandBuffers(device.logical, &allocInfo, &commandBuffer));
        return commandBuffer;
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
