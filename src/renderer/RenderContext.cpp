#include "renderer/RenderContext.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Pipeline.hpp"
#include "renderer/vulkan/Validation.hpp"
#include "renderer/TextureManager.hpp"
#include "Utils.hpp"

bool RenderContext::Init(const char *title, int x, int y, int w, int h)
{
    window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

    if (!initVulkan())
        return false;

    SDL_GetWindowSize(window, &width, &height);

    halfWidth  = width  >> 1;
    halfHeight = height >> 1;

    scrRatio = (float)width / (float)height;

/*
 *       ----(1.0)---
 *       |          |
 *    -ratio      ratio
 *       |          |
 *       ---(-1.0)---
 */
    left   = -scrRatio;
    right  = scrRatio;
    bottom = -1.0f;
    top    = 1.0f;

    return true;
}

void RenderContext::Destroy()
{
    if (window)
    {
        vkDeviceWaitIdle(device.logical);
        destroyFramebuffers();
        destroyImageViews();
        if (m_depthBuffer.image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(device.allocator, m_depthBuffer.image, m_depthBuffer.allocation);
            vkDestroyImageView(device.logical, m_depthBuffer.imageView, nullptr);
        }

        TextureManager::GetInstance()->ReleaseTextures();

        vkDestroySwapchainKHR(device.logical, swapChain.sc, nullptr);
        vkDestroySemaphore(device.logical, m_imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device.logical, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device.logical, renderUIFinishedSemaphore, nullptr);
        vk::destroyAllocator(device.allocator);
        vkDestroyDevice(device.logical, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
#ifdef VALIDATION_LAYERS_ON
        vk::destroyValidationLayers(m_instance);
#endif
        vkDestroyInstance(m_instance, nullptr);
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

bool RenderContext::initVulkan()
{
    VK_VERIFY(vk::createInstance(window, &m_instance, "Quake BSP Viewer in Vulkan"));
    // "oldschool" way of creating a Vulkan surface in SDL prior to 2.0.6
    //VK_VERIFY(vk::createSurface(window, m_instance, &m_surface));
    SDL_Vulkan_CreateSurface(window, m_instance, &m_surface);

    device = vk::createDevice(m_instance, m_surface);
    VK_VERIFY(vk::createAllocator(device, &device.allocator));

    swapChain = vk::createSwapChain(device, m_surface, VK_NULL_HANDLE);
    if (swapChain.sc == VK_NULL_HANDLE) return false;

    if (!createImageViews()) return false;
    createSemaphores();

    return true;
}

bool RenderContext::createImageViews()
{
    m_imageViews.resize(swapChain.images.size());

    for (size_t i = 0; i < swapChain.images.size(); ++i)
    {
        VkResult result = vk::createImageView(device, swapChain.images[i], VK_IMAGE_ASPECT_COLOR_BIT, &m_imageViews[i], swapChain.format);

        if (result != VK_SUCCESS)
        {
            VK_VERIFY(result);
            destroyImageViews();
            return false;
        }
    }

    return true;
}

void RenderContext::destroyImageViews()
{
    for (VkImageView &iv : m_imageViews)
        vkDestroyImageView(device.logical, iv, nullptr);
}

bool RenderContext::createFramebuffers(const vk::RenderPass &renderPass)
{
    frameBuffers.resize(m_imageViews.size());

    for (size_t i = 0; i < m_imageViews.size(); ++i)
    {
        VkImageView attachments[] = { m_imageViews[i], m_depthBuffer.imageView };

        VkFramebufferCreateInfo fbCreateInfo = {};
        fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCreateInfo.renderPass = renderPass.renderPass;
        fbCreateInfo.attachmentCount = 2;
        fbCreateInfo.pAttachments = attachments;
        fbCreateInfo.width = swapChain.extent.width;
        fbCreateInfo.height = swapChain.extent.height;
        fbCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(device.logical, &fbCreateInfo, nullptr, &frameBuffers[i]);

        if (result != VK_SUCCESS)
        {
            VK_VERIFY(result);
            destroyFramebuffers();
            return false;
        }
    }

    return true;
}

void RenderContext::destroyFramebuffers()
{
    for (VkImage &fb : frameBuffers)
    {
        vkDestroyFramebuffer(device.logical, fb, nullptr);
    }

    frameBuffers.clear();
}

void RenderContext::createSemaphores()
{
    VkSemaphoreCreateInfo sCreateInfo = {};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &m_imageAvailableSemaphore));
    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &renderFinishedSemaphore));
    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &renderUIFinishedSemaphore));
}

VkResult RenderContext::RenderStart()
{
#undef max
    vkQueueWaitIdle(device.presentQueue);
    VkResult result = vkAcquireNextImageKHR(device.logical, swapChain.sc, std::numeric_limits<int>::max(), m_imageAvailableSemaphore, VK_NULL_HANDLE, &m_imageIndex);

    // swapchain has become incompatible - application has to recreate it
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return result;

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_imageAvailableSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    LOG_MESSAGE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Could not acquire swapchain image: " << result);
    return result;
}

VkResult RenderContext::Submit(const std::vector<VkCommandBuffer> &commandBuffers)
{
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[m_imageIndex];

    return vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

VkResult RenderContext::Present(bool uiVisible)
{
    VkSwapchainKHR swapChains[] = { swapChain.sc };
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = uiVisible ? &renderUIFinishedSemaphore : &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_imageIndex;
    presentInfo.pResults = nullptr;

    return vkQueuePresentKHR(device.graphicsQueue, &presentInfo);
}

bool RenderContext::recreateSwapChain(const VkCommandPool &commandPool, const vk::RenderPass &renderPass)
{
    vkDeviceWaitIdle(device.logical);
    destroyFramebuffers();
    destroyImageViews();

    VkSwapchainKHR oldSwapChain = swapChain.sc;
    swapChain = vk::createSwapChain(device, m_surface, oldSwapChain);
    vkDestroySwapchainKHR(device.logical, oldSwapChain, nullptr);
    if (swapChain.sc == VK_NULL_HANDLE) return false;

    if (!createImageViews()) return false;

    if (m_depthBuffer.image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(device.allocator, m_depthBuffer.image, m_depthBuffer.allocation);
        vkDestroyImageView(device.logical, m_depthBuffer.imageView, nullptr);
    }
    m_depthBuffer = vk::createDepthBuffer(device, swapChain, commandPool);

    if (!createFramebuffers(renderPass)) return false;

    return true;
}
