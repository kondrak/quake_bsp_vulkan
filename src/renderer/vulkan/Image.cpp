#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Image.hpp"
#include "renderer/vulkan/Buffers.hpp"
#include "Utils.hpp"

namespace vk
{
    // internal helpers
    static void transitionImageLayout(const Device &device, const VkCommandBuffer &cmdBuffer, const VkQueue &queue, const Texture &texture, const VkImageLayout &oldLayout, const VkImageLayout &newLayout);
    static void copyBufferToImage(const VkCommandBuffer &cmdBuffer, const VkBuffer &buffer, const VkImage &image, uint32_t width, uint32_t height);
    static VkResult createImage(const Device &device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, Texture *texture);
    static void generateMipmaps(const VkCommandBuffer &cmdBuffer, const Texture &texture, uint32_t width, uint32_t height);
    static VkImageAspectFlags getDepthStencilAspect(VkFormat depthFormat);

    void createTextureImage(const Device &device, Texture *dstTex, const unsigned char *data, uint32_t width, uint32_t height)
    {
        Buffer stagingBuffer;
        bool unifiedTransferAndGfx = device.transferQueue == device.graphicsQueue;
        uint32_t imageSize = width * height * (dstTex->format == VK_FORMAT_R8G8B8_UNORM ? 3 : 4);

        VK_VERIFY(createStagingBuffer(device, imageSize, &stagingBuffer));

        void *imgData;
        vmaMapMemory(device.allocator, stagingBuffer.allocation, &imgData);
        memcpy(imgData, data, (size_t)imageSize);
        vmaUnmapMemory(device.allocator, stagingBuffer.allocation);

        VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        // set extra image usage flag if we're dealing with mipmapped image - will need it for copying data between mip levels
        if (dstTex->mipLevels > 1)
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VK_VERIFY(createImage(device, width, height, dstTex->format, VK_IMAGE_TILING_OPTIMAL, imageUsage, VMA_MEMORY_USAGE_GPU_ONLY, dstTex));

        // copy buffers
        VkCommandBuffer transferCmdBuffer = createCommandBuffer(device, device.transferCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VkCommandBuffer drawCmdBuffer = unifiedTransferAndGfx ? transferCmdBuffer : createCommandBuffer(device, device.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        beginCommand(transferCmdBuffer);
        transitionImageLayout(device, transferCmdBuffer, device.transferQueue, *dstTex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(transferCmdBuffer, stagingBuffer.buffer, dstTex->image, width, height);

        if (dstTex->mipLevels > 1)
        {
            // if transfer and graphics queue are different we have to submit the transfer queue before continuing
            if (!unifiedTransferAndGfx)
            {
                submitCommand(device, transferCmdBuffer, device.transferQueue);
                beginCommand(drawCmdBuffer);
            }

            // vkCmdBlitImage requires a queue with GRAPHICS_BIT present
            generateMipmaps(drawCmdBuffer, *dstTex, width, height);
            submitCommand(device, drawCmdBuffer, device.graphicsQueue);
        }
        else
        {
            // for non-unified transfer and graphics, this step begins queue ownership transfer to graphics queue (for exclusive sharing only)
            if(unifiedTransferAndGfx || dstTex->sharingMode == VK_SHARING_MODE_EXCLUSIVE)
                transitionImageLayout(device, transferCmdBuffer, device.transferQueue, *dstTex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            submitCommand(device, transferCmdBuffer, device.transferQueue);

            if (!unifiedTransferAndGfx)
            {
                beginCommand(drawCmdBuffer);
                transitionImageLayout(device, drawCmdBuffer, device.graphicsQueue, *dstTex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                submitCommand(device, drawCmdBuffer, device.graphicsQueue);
            }
        }

        vkFreeCommandBuffers(device.logical, device.transferCommandPool, 1, &transferCmdBuffer);
        if(!unifiedTransferAndGfx)
            vkFreeCommandBuffers(device.logical, device.commandPool, 1, &drawCmdBuffer);

        freeBuffer(device, stagingBuffer);
    }

    void createTexture(const Device &device, Texture *dstTex, const unsigned char *data, uint32_t width, uint32_t height)
    {
        createTextureImage(device, dstTex, data, width, height);
        VK_VERIFY(createImageView(device, dstTex->image, VK_IMAGE_ASPECT_COLOR_BIT, &dstTex->imageView, dstTex->format, dstTex->mipLevels));
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

    VkResult createImageView(const Device &device, const VkImage &image, VkImageAspectFlags aspectFlags, VkImageView *imageView, VkFormat format, uint32_t mipLevels)
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
        ivCreateInfo.subresourceRange.levelCount = mipLevels;

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
        if (device.properties.limits.maxSamplerAnisotropy == 1.f)
            samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = texture->mipmapMode;
        samplerInfo.mipLodBias = texture->mipLodBias;
        samplerInfo.minLod = texture->mipMinLod;
        samplerInfo.maxLod = (float)texture->mipLevels;

        return vkCreateSampler(device.logical, &samplerInfo, nullptr, &texture->sampler);
    }

    // helper functions
    Texture createColorBuffer(const Device &device, const SwapChain &swapChain, VkSampleCountFlagBits sampleCount)
    {
        Texture colorTexture;
        colorTexture.format = swapChain.format;
        colorTexture.sampleCount = sampleCount;

        VK_VERIFY(createImage(device, swapChain.extent.width, swapChain.extent.height, colorTexture.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &colorTexture));
        VK_VERIFY(createImageView(device, colorTexture.image, VK_IMAGE_ASPECT_COLOR_BIT, &colorTexture.imageView, colorTexture.format, colorTexture.mipLevels));

        VkCommandBuffer cmdBuffer = createCommandBuffer(device, device.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        beginCommand(cmdBuffer);
        transitionImageLayout(device, cmdBuffer, device.graphicsQueue, colorTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        submitCommand(device, cmdBuffer, device.graphicsQueue);
        vkFreeCommandBuffers(device.logical, device.commandPool, 1, &cmdBuffer);

        return colorTexture;
    }

    Texture createDepthBuffer(const Device &device, const SwapChain &swapChain, VkSampleCountFlagBits sampleCount)
    {
        Texture depthTexture;
        depthTexture.format = getBestDepthFormat(device);
        depthTexture.sampleCount = sampleCount;

        VK_VERIFY(createImage(device, swapChain.extent.width, swapChain.extent.height, depthTexture.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &depthTexture));
        VK_VERIFY(createImageView(device, depthTexture.image, getDepthStencilAspect(depthTexture.format), &depthTexture.imageView, depthTexture.format, depthTexture.mipLevels));

        VkCommandBuffer cmdBuffer = createCommandBuffer(device, device.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        beginCommand(cmdBuffer);
        transitionImageLayout(device, cmdBuffer, device.graphicsQueue, depthTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        submitCommand(device, cmdBuffer, device.graphicsQueue);
        vkFreeCommandBuffers(device.logical, device.commandPool, 1, &cmdBuffer);

        return depthTexture;
    }

    void transitionImageLayout(const Device &device, const VkCommandBuffer &cmdBuffer, const VkQueue &queue, const Texture &texture, const VkImageLayout &oldLayout, const VkImageLayout &newLayout)
    {
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
        imgBarrier.subresourceRange.levelCount = texture.mipLevels;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imgBarrier.subresourceRange.aspectMask = getDepthStencilAspect(texture.format);
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
            if (device.transferQueue == device.graphicsQueue)
            {
                imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else
            {
                if (device.transferQueue == queue)
                {
                    // if the image is exclusively shared, start queue ownership transfer process (release) - only for VK_SHARING_MODE_EXCLUSIVE
                    imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imgBarrier.dstAccessMask = 0;
                    imgBarrier.srcQueueFamilyIndex = device.transferFamilyIndex;
                    imgBarrier.dstQueueFamilyIndex = device.graphicsFamilyIndex;
                    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                }
                else
                {
                    // continuing queue transfer (acquisition) - this will only happen for VK_SHARING_MODE_EXCLUSIVE images
                    if (texture.sharingMode == VK_SHARING_MODE_EXCLUSIVE)
                    {
                        imgBarrier.srcAccessMask = 0;
                        imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        imgBarrier.srcQueueFamilyIndex = device.transferFamilyIndex;
                        imgBarrier.dstQueueFamilyIndex = device.graphicsFamilyIndex;
                        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    }
                    else
                    {
                        imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    }
                }
            }
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
    }

    void copyBufferToImage(const VkCommandBuffer &cmdBuffer, const VkBuffer &buffer, const VkImage &image, uint32_t width, uint32_t height)
    {
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
    }

    VkResult createImage(const Device &device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, Texture *texture)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = texture->mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = texture->sampleCount;
        imageInfo.flags = 0;

        uint32_t queueFamilies[] = { (uint32_t)device.graphicsFamilyIndex, (uint32_t)device.transferFamilyIndex };
        if (device.graphicsFamilyIndex != device.transferFamilyIndex)
        {
            imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            imageInfo.queueFamilyIndexCount = 2;
            imageInfo.pQueueFamilyIndices = queueFamilies;
        }

        VmaAllocationCreateInfo vmallocInfo = {};
        // make sure memory regions for loaded images do not overlap
        vmallocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmallocInfo.usage = memUsage;

        texture->sharingMode = imageInfo.sharingMode;
        return vmaCreateImage(device.allocator, &imageInfo, &vmallocInfo, &texture->image, &texture->allocation, nullptr);
    }

    void generateMipmaps(const VkCommandBuffer &cmdBuffer, const Texture &texture, uint32_t width, uint32_t height)
    {
        int32_t mipWidth  = width;
        int32_t mipHeight = height;

        VkImageMemoryBarrier imgBarrier = {};
        imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgBarrier.image = texture.image;
        imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgBarrier.subresourceRange.baseArrayLayer = 0;
        imgBarrier.subresourceRange.layerCount = 1;
        imgBarrier.subresourceRange.levelCount = 1;

        // copy rescaled mip data between consecutive levels (each higher level is half the size of the previous level)
        for (uint32_t i = 1; i < texture.mipLevels; ++i)
        {
            imgBarrier.subresourceRange.baseMipLevel = i - 1;
            imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);

            VkImageBlit blit = {};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ?  mipWidth >> 1 : 1,
                                  mipHeight > 1 ? mipHeight >> 1 : 1, 1 }; // each mip level is half the size of the previous level
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            // src image == dst image, because we're blitting between different mip levels of the same image
            vkCmdBlitImage(cmdBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, texture.mipmapFilter);

            imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);

            // avoid zero-sized mip levels
            if ( mipWidth > 1)  mipWidth >>= 1;
            if (mipHeight > 1) mipHeight >>= 1;
        }

        imgBarrier.subresourceRange.baseMipLevel = texture.mipLevels - 1;
        imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);
    }

    VkImageAspectFlags getDepthStencilAspect(VkFormat depthFormat)
    {
        switch (depthFormat)
        {
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
    }
}
