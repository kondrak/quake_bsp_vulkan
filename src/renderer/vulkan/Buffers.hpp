#pragma once

#include "renderer/vulkan/Base.hpp"
#include "renderer/vulkan/Image.hpp"
#include <vector>

/*
 *  Creation and release of common Vulkan buffers
 */

namespace vk
{
    // Vulkan buffer with assigned allocator
    struct Buffer
    {
        VkBuffer      buffer     = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    // helper struct
    struct BufferOptions
    {
        VkBufferUsageFlags usage = 0;
        VkMemoryPropertyFlags memFlags = 0;
        VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_UNKNOWN;
        VmaAllocationCreateFlags vmaFlags = 0;
    };

    // helper struct
    struct VertexBufferInfo
    {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    };


    VkVertexInputBindingDescription getBindingDescription(uint32_t stride);
    VkVertexInputAttributeDescription getAttributeDescription(uint32_t location, VkFormat format, uint32_t offset);
    // shader buffers and generic Vulkan buffer creation
    VkResult createBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer, const BufferOptions &bOpts);
    void     freeBuffer(const Device &device, Buffer &buffer);
    VkResult createStagingBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer);
    void     createVertexBuffer(const Device &device, const void *data, VkDeviceSize size, Buffer *dstBuffer);
    void     createVertexBufferStaged(const Device &device, VkDeviceSize size, const Buffer &stagingBuffer, Buffer *dstBuffer);
    void     createIndexBuffer(const Device &device, const void *data, VkDeviceSize size, Buffer *dstBuffer);
    void     createIndexBufferStaged(const Device &device, VkDeviceSize size, const Buffer &stagingBuffer, Buffer *dstBuffer);
    VkResult createUniformBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer);
}
