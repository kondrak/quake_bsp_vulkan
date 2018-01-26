#pragma once

#include "renderer/vulkan/Base.hpp"
#include "renderer/vulkan/Image.hpp"
#include <vector>

namespace vk
{
    struct Buffer
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    struct BufferOptions
    {
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags memFlags;
        VmaMemoryUsage vmaUsage;
        VmaAllocationCreateFlags vmaFlags = 0;
    };

    struct VertexBufferInfo
    {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    };

    VkResult initAllocator(Device &device);
    void destroyAllocator(Device &device);
    VkVertexInputBindingDescription getBindingDescription(uint32_t stride);
    VkVertexInputAttributeDescription getAttributeDescription(uint32_t location, VkFormat format, uint32_t offset);
    VkResult createBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer, const BufferOptions &bOpts);
    void freeBuffer(const Device &device, Buffer *buffer);
    void copyBuffer(const Device &device, const VkCommandPool &commandPool, const VkBuffer &src, VkBuffer &dst, VkDeviceSize size);
    VkResult createStagingBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer);
    void createVertexBuffer(const Device &device, const VkCommandPool &commandPool, const void *data, VkDeviceSize size, Buffer *dstBuffer);
    void createIndexBuffer(const Device &device, const VkCommandPool &commandPool, const void *data, VkDeviceSize size, Buffer *dstBuffer);
    VkResult createUniformBuffer(const Device &device, VkDeviceSize size, Buffer *dstBuffer);

    VkResult createDescriptorSet(const Device &device, Descriptor *descriptor);
}
