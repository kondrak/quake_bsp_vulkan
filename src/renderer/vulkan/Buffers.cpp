#include "renderer/vulkan/Buffers.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "Utils.hpp"
#include "renderer/vulkan/vk_mem_alloc.h"

namespace vk
{
    static void copyBuffer(const Device &device, const VkBuffer &src, VkBuffer &dst, VkDeviceSize size);

    VkVertexInputBindingDescription getBindingDescription(uint32_t stride)
    {
        VkVertexInputBindingDescription bindingDesc = {};
        bindingDesc.binding = 0;
        bindingDesc.stride = stride;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDesc;
    }

    VkVertexInputAttributeDescription getAttributeDescription(uint32_t location, VkFormat format, uint32_t offset)
    {
        VkVertexInputAttributeDescription attributeDesc = {};
        attributeDesc.binding = 0;
        attributeDesc.location = location;
        attributeDesc.format = format;
        attributeDesc.offset = offset;
        return attributeDesc;
    }

    VkResult createBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer, const BufferOptions &bOpts)
    {
        VkBufferCreateInfo bcInfo = {};
        bcInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bcInfo.size = size;
        bcInfo.usage = bOpts.usage;
        bcInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // separate transfer queue makes sense only if the buffer is targetted for being transfered to GPU, so ignore it if it's CPU-only
        if (bOpts.vmaUsage != VMA_MEMORY_USAGE_CPU_ONLY && device.graphicsFamilyIndex != device.transferFamilyIndex)
        {
            uint32_t queueFamilies[] = { (uint32_t)device.graphicsFamilyIndex, (uint32_t)device.transferFamilyIndex };
            bcInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bcInfo.queueFamilyIndexCount = 2;
            bcInfo.pQueueFamilyIndices = queueFamilies;
        }

        VmaAllocationCreateInfo vmallocInfo = {};
        vmallocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        vmallocInfo.preferredFlags = bOpts.memFlags;
        vmallocInfo.flags = bOpts.vmaFlags;
        vmallocInfo.usage = bOpts.vmaUsage;

        return vmaCreateBuffer(device.allocator, &bcInfo, &vmallocInfo, &dstBuffer->buffer, &dstBuffer->allocation, nullptr);
    }

    void freeBuffer(const Device &device, Buffer &buffer)
    {
        vmaDestroyBuffer(device.allocator, buffer.buffer, buffer.allocation);
    }

    VkResult createStagingBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer)
    {
        BufferOptions stagingOpts;
        stagingOpts.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingOpts.memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        stagingOpts.vmaFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        stagingOpts.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
        return createBuffer(device, size, dstBuffer, stagingOpts);
    }

    void createVertexBuffer(const Device &device, const void *data, VkDeviceSize size, Buffer *dstBuffer)
    {
        Buffer stagingBuffer;
        VK_VERIFY(createStagingBuffer(device, size, &stagingBuffer));

        void *dst;
        vmaMapMemory(device.allocator, stagingBuffer.allocation, &dst);
        memcpy(dst, data, (size_t)size);
        vmaUnmapMemory(device.allocator, stagingBuffer.allocation);

        BufferOptions dstOpts;
        dstOpts.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        dstOpts.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        dstOpts.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_VERIFY(createBuffer(device, size, dstBuffer, dstOpts));

        copyBuffer(device, stagingBuffer.buffer, dstBuffer->buffer, size);
        freeBuffer(device, stagingBuffer);
    }

    void createVertexBufferStaged(const Device &device, VkDeviceSize size, const Buffer &stagingBuffer, Buffer *dstBuffer)
    {
        BufferOptions dstOpts;
        dstOpts.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        dstOpts.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        dstOpts.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_VERIFY(createBuffer(device, size, dstBuffer, dstOpts));

        copyBuffer(device, stagingBuffer.buffer, dstBuffer->buffer, size);
    }

    void createIndexBuffer(const Device &device, const void *data, VkDeviceSize size, Buffer *dstBuffer)
    {
        Buffer stagingBuffer;
        VK_VERIFY(createStagingBuffer(device, size, &stagingBuffer));

        void *dst;
        vmaMapMemory(device.allocator, stagingBuffer.allocation, &dst);
        memcpy(dst, data, (size_t)size);
        vmaUnmapMemory(device.allocator, stagingBuffer.allocation);

        BufferOptions dstOpts;
        dstOpts.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        dstOpts.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        dstOpts.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_VERIFY(createBuffer(device, size, dstBuffer, dstOpts));

        copyBuffer(device, stagingBuffer.buffer, dstBuffer->buffer, size);
        freeBuffer(device, stagingBuffer);
    }

    void createIndexBufferStaged(const Device &device, VkDeviceSize size, const Buffer &stagingBuffer, Buffer *dstBuffer)
    {
        BufferOptions dstOpts;
        dstOpts.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        dstOpts.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        dstOpts.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_VERIFY(createBuffer(device, size, dstBuffer, dstOpts));

        copyBuffer(device, stagingBuffer.buffer, dstBuffer->buffer, size);
    }

    VkResult createUniformBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer)
    {
        BufferOptions dstOpts;
        dstOpts.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        dstOpts.memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        dstOpts.vmaFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        dstOpts.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        return createBuffer(device, size, dstBuffer, dstOpts);
    }

    // internal helper
    void copyBuffer(const Device &device, const VkBuffer &src, VkBuffer &dst, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = createCommandBuffer(device, device.transferCommandPool);
        beginCommand(commandBuffer);

        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

        submitCommand(device, commandBuffer, device.transferQueue);
        vkFreeCommandBuffers(device.logical, device.transferCommandPool, 1, &commandBuffer);
    }
}
