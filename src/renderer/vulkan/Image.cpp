#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Image.hpp"
#include "renderer/vulkan/Buffers.hpp"
#include "Utils.hpp"

namespace vk
{
    // internal helpers
    static void transitionImageLayout(const Device &device, const VkCommandPool &commandPool, const Texture &texture, const VkImageLayout &oldLayout, const VkImageLayout &newLayout);
    static void copyBufferToImage(const Device &device, const VkCommandPool &commandPool, const VkBuffer &buffer, const VkImage &image, uint32_t width, uint32_t height);
    static VkResult createImage(const Device &device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, Texture *texture);

    void createTextureImage(const Device &device, const VkCommandPool &commandPool, Texture *dstTex, const unsigned char *data, uint32_t width, uint32_t height)
    {
        Buffer stagingBuffer;
        uint32_t imageSize = width * height * (dstTex->format == VK_FORMAT_R8G8B8_UNORM ? 3 : 4);

        VK_VERIFY(createStagingBuffer(device, imageSize, &stagingBuffer));

        void *imgData;
        vmaMapMemory(device.allocator, stagingBuffer.allocation, &imgData);
        memcpy(imgData, data, (size_t)imageSize);
        vmaUnmapMemory(device.allocator, stagingBuffer.allocation);

        VK_VERIFY(createImage(device, width, height, dstTex->format,
                              VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, dstTex));
        // copy buffers
        transitionImageLayout(device, commandPool, *dstTex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(device, commandPool, stagingBuffer.buffer, dstTex->image, width, height);
        transitionImageLayout(device, commandPool, *dstTex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        freeBuffer(device, stagingBuffer);
    }

    void createTexture(const Device &device, const VkCommandPool &commandPool, Texture *dstTex, const unsigned char *data, uint32_t width, uint32_t height)
    {
        createTextureImage(device, commandPool, dstTex, data, width, height);
        VK_VERIFY(createImageView(device, dstTex->image, VK_IMAGE_ASPECT_COLOR_BIT, &dstTex->imageView, dstTex->format));
        VK_VERIFY(createTextureSampler(device, dstTex));
    }

    void releaseTexture(const Device &device, Texture &texture)
    {
        if (texture.image != VK_NULL_HANDLE)
            vmaDestroyImage(device.allocator, texture.image, texture.allocation);
        if (texture.imageView != VK_NULL_HANDLE)
            vkDestroyImageView(device.logical, texture.imageView, nullptr);
        if (texture.sampler != VK_NULL_HANDLE)
            vkDestroySampler(device.logical, texture.sampler, nullptr);
    }

    VkResult createImageView(const Device &device, const VkImage &image, VkImageAspectFlags aspectFlags, VkImageView *imageView, VkFormat format)
    {
        VkImageViewCreateInfo ivCreateInfo = {};
        ivCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivCreateInfo.image = image;
        ivCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivCreateInfo.format = format;
        ivCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivCreateInfo.subresourceRange.aspectMask = aspectFlags;
        ivCreateInfo.subresourceRange.baseArrayLayer = 0;
        ivCreateInfo.subresourceRange.baseMipLevel = 0;
        ivCreateInfo.subresourceRange.layerCount = 1;
        ivCreateInfo.subresourceRange.levelCount = 1;

        return vkCreateImageView(device.logical, &ivCreateInfo, nullptr, imageView);
    }

    VkResult createTextureSampler(const Device &device, Texture *texture)
    {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = texture->magFilter;
        samplerInfo.minFilter = texture->minFilter;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = (texture->magFilter == texture->minFilter) && texture->minFilter == VK_FILTER_NEAREST ? VK_FALSE : VK_TRUE;
        samplerInfo.maxAnisotropy = 16;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.f;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = 0.f;

        return vkCreateSampler(device.logical, &samplerInfo, nullptr, &texture->sampler);
    }

    // helper functions
    Texture createColorBuffer(const Device &device, const SwapChain &swapChain, const VkCommandPool &commandPool, VkSampleCountFlagBits sampleCount)
    {
        Texture colorTexture;
        colorTexture.format = swapChain.format;
        colorTexture.sampleCount = sampleCount;

        VK_VERIFY(createImage(device, swapChain.extent.width, swapChain.extent.height, colorTexture.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &colorTexture));
        VK_VERIFY(createImageView(device, colorTexture.image, VK_IMAGE_ASPECT_COLOR_BIT, &colorTexture.imageView, colorTexture.format));

        transitionImageLayout(device, commandPool, colorTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        return colorTexture;
    }

    Texture createDepthBuffer(const Device &device, const SwapChain &swapChain, const VkCommandPool &commandPool, VkSampleCountFlagBits sampleCount)
    {
        Texture depthTexture;
        depthTexture.format = VK_FORMAT_D32_SFLOAT;
        depthTexture.sampleCount = sampleCount;

        VK_VERIFY(createImage(device, swapChain.extent.width, swapChain.extent.height, depthTexture.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &depthTexture));
        VK_VERIFY(createImageView(device, depthTexture.image, VK_IMAGE_ASPECT_DEPTH_BIT, &depthTexture.imageView, depthTexture.format));

        transitionImageLayout(device, commandPool, depthTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        return depthTexture;
    }

    void transitionImageLayout(const Device &device, const VkCommandPool &commandPool, const Texture &texture, const VkImageLayout &oldLayout, const VkImageLayout &newLayout)
    {
        VkCommandBuffer cmdBuffer = beginOneTimeCommand(device, commandPool);
        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;

        VkImageMemoryBarrier imgBarrier = {};
        imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imgBarrier.oldLayout = oldLayout;
        imgBarrier.newLayout = newLayout;
        imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgBarrier.image = texture.image;
        imgBarrier.subresourceRange.baseMipLevel = 0; // no mip mapping levels
        imgBarrier.subresourceRange.baseArrayLayer = 0;
        imgBarrier.subresourceRange.layerCount = 1;
        imgBarrier.subresourceRange.levelCount = 1;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imgBarrier.srcAccessMask = 0;
            imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imgBarrier.srcAccessMask = 0;
            imgBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            imgBarrier.srcAccessMask = 0;
            imgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        else
        {
            LOG_MESSAGE_ASSERT(false, "Invalid stage");
        }

        vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);

        endOneTimeCommand(device, cmdBuffer, commandPool, device.graphicsQueue);
    }

    void copyBufferToImage(const Device &device, const VkCommandPool &commandPool, const VkBuffer &buffer, const VkImage &image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer cmdBuffer = beginOneTimeCommand(device, commandPool);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endOneTimeCommand(device, cmdBuffer, commandPool, device.graphicsQueue);
    }

    VkResult createImage(const Device &device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, Texture *texture)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1; // no mipmapping
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = texture->sampleCount;
        imageInfo.flags = 0;

        VmaAllocationCreateInfo vmallocInfo = {};
        vmallocInfo.usage = memUsage;

        return vmaCreateImage(device.allocator, &imageInfo, &vmallocInfo, &texture->image, &texture->allocation, nullptr);
    }
}
