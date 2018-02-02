#include "renderer/RenderContext.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Pipeline.hpp"
#include "renderer/vulkan/Validation.hpp"
#include "renderer/TextureManager.hpp"
#include "Utils.hpp"
#include <algorithm>

bool RenderContext::Init(const char *title, int x, int y, int w, int h)
{
    window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

    SDL_GetWindowSize(window, &width, &height);

    if (!InitVulkan())
        return false;

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
        DestroyFramebuffers();
        DestroyImageViews();
        DestroyDepthBuffer();

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

VkResult RenderContext::Submit(const vk::CmdBufferList &commandBuffers)
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

Math::Vector2f RenderContext::WindowSize()
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical, m_surface, &surfaceCaps);
    LOG_MESSAGE_ASSERT(surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max(), "WM sets extent width and height to max uint32!");

    // fallback if WM sets extent dimensions to max uint32
    if (surfaceCaps.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
#undef min
#undef max
        // fetch current window width and height from SDL, since we can't rely on WM in this case
        SDL_GetWindowSize(window, &width, &height);
        float w = (float)std::max(surfaceCaps.minImageExtent.width,  std::min(surfaceCaps.maxImageExtent.width,  (uint32_t)width));
        float h = (float)std::max(surfaceCaps.minImageExtent.height, std::min(surfaceCaps.maxImageExtent.height, (uint32_t)height));
        return Math::Vector2f(w, h);
    }

    return Math::Vector2f((float)surfaceCaps.currentExtent.width, (float)surfaceCaps.currentExtent.height);
}

bool RenderContext::RecreateSwapChain(const VkCommandPool &commandPool, const vk::RenderPass &renderPass)
{
    vkDeviceWaitIdle(device.logical);
    DestroyFramebuffers();
    DestroyImageViews();

    // set initial swap chain extent to current window size - in case WM won't be able to determine it by itself
    swapChain.extent = { (uint32_t)width, (uint32_t)height };
    VK_VERIFY(vk::createSwapChain(device, m_surface, &swapChain, swapChain.sc));

    DestroyDepthBuffer();
    CreateDepthBuffer(commandPool);
    if (!CreateImageViews()) return false;
    if (!CreateFramebuffers(renderPass)) return false;

    return true;
}

bool RenderContext::InitVulkan()
{
    VK_VERIFY(vk::createInstance(window, &m_instance, "Quake BSP Viewer in Vulkan"));
    // "oldschool" way of creating a Vulkan surface in SDL prior to 2.0.6
    //VK_VERIFY(vk::createSurface(window, m_instance, &m_surface));
    SDL_Vulkan_CreateSurface(window, m_instance, &m_surface);

    device = vk::createDevice(m_instance, m_surface);
    VK_VERIFY(vk::createAllocator(device, &device.allocator));
    // set initial swap chain extent to current window size - in case WM won't be able to determine it by itself
    swapChain.extent = { (uint32_t)width, (uint32_t)height };
    VK_VERIFY(vk::createSwapChain(device, m_surface, &swapChain, VK_NULL_HANDLE));

    if (!CreateImageViews()) return false;
    CreateSemaphores();

    return true;
}

void RenderContext::CreateDepthBuffer(const VkCommandPool &commandPool)
{
    m_depthBuffer = vk::createDepthBuffer(device, swapChain, commandPool);
}

void RenderContext::DestroyDepthBuffer()
{
    if (m_depthBuffer.image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(device.allocator, m_depthBuffer.image, m_depthBuffer.allocation);
        vkDestroyImageView(device.logical, m_depthBuffer.imageView, nullptr);
    }
}

bool RenderContext::CreateImageViews()
{
    m_imageViews.resize(swapChain.images.size());

    for (size_t i = 0; i < swapChain.images.size(); ++i)
    {
        VkResult result = vk::createImageView(device, swapChain.images[i], VK_IMAGE_ASPECT_COLOR_BIT, &m_imageViews[i], swapChain.format);

        if (result != VK_SUCCESS)
        {
            VK_VERIFY(result);
            DestroyImageViews();
            return false;
        }
    }

    return true;
}

void RenderContext::DestroyImageViews()
{
    for (VkImageView &iv : m_imageViews)
        vkDestroyImageView(device.logical, iv, nullptr);
}

bool RenderContext::CreateFramebuffers(const vk::RenderPass &renderPass)
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
            DestroyFramebuffers();
            return false;
        }
    }

    return true;
}

void RenderContext::DestroyFramebuffers()
{
    for (VkImage &fb : frameBuffers)
    {
        vkDestroyFramebuffer(device.logical, fb, nullptr);
    }

    frameBuffers.clear();
}

void RenderContext::CreateSemaphores()
{
    VkSemaphoreCreateInfo sCreateInfo = {};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &m_imageAvailableSemaphore));
    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &renderFinishedSemaphore));
    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &renderUIFinishedSemaphore));
}
