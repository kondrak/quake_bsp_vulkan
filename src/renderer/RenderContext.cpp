#include "renderer/RenderContext.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Pipeline.hpp"
#include "renderer/vulkan/Validation.hpp"
#include "renderer/TextureManager.hpp"
#include "Utils.hpp"
#include <algorithm>

// use 2 synchronized command buffers for rendering (double buffering)
static const int NUM_CMDBUFFERS = 2;

bool RenderContext::Init(const char *title, int x, int y, int w, int h)
{
    window = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    SDL_GetWindowSize(window, &width, &height);

    halfWidth  = width  >> 1;
    halfHeight = height >> 1;
    scrRatio   = (float)width / (float)height;
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

    return InitVulkan();
}

void RenderContext::Destroy()
{
    if (window)
    {
        vkDeviceWaitIdle(device.logical);

        vk::destroyRenderPass(device, renderPass);
        vk::freeCommandBuffers(device, commandPool, m_commandBuffers);
        vkDestroyCommandPool(device.logical, commandPool, nullptr);
 
        DestroyFramebuffers();
        DestroyImageViews();
        DestroyDepthBuffer();

        TextureManager::GetInstance()->ReleaseTextures();

        vkDestroySwapchainKHR(device.logical, swapChain.sc, nullptr);

        vkDestroySemaphore(device.logical, m_imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device.logical, m_renderFinishedSemaphore, nullptr);
        for (VkFence &fence : m_fences)
        {
            vkDestroyFence(device.logical, fence, nullptr);
        }

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
    VkResult result = vkAcquireNextImageKHR(device.logical, swapChain.sc, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &m_imageIndex);
    activeCmdBuffer = m_commandBuffers[m_imageIndex];
 
    // swapchain has become incompatible - application has to recreate it
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return result;

    VK_VERIFY(vkWaitForFences(device.logical, 1, &m_fences[m_imageIndex], VK_TRUE, UINT64_MAX));
    vkResetFences(device.logical, 1, &m_fences[m_imageIndex]);

    LOG_MESSAGE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Could not acquire swapchain image: " << result);

    // setup command buffers and render pass for drawing
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    result = vkBeginCommandBuffer(m_commandBuffers[m_imageIndex], &beginInfo);
    LOG_MESSAGE_ASSERT(result == VK_SUCCESS, "Could not begin command buffer: " << result);

    VkClearValue clearColors[2];
    clearColors[0].color = { 0.f, 0.f, 0.f, 1.f };
    clearColors[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderBeginInfo = {};
    renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBeginInfo.renderPass = renderPass.renderPass;
    renderBeginInfo.framebuffer = m_frameBuffers[m_imageIndex];
    renderBeginInfo.renderArea.offset = { 0, 0 };
    renderBeginInfo.renderArea.extent = swapChain.extent;
    renderBeginInfo.clearValueCount = 2;
    renderBeginInfo.pClearValues = clearColors;

    vkCmdBeginRenderPass(m_commandBuffers[m_imageIndex], &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    return VK_SUCCESS;
}

VkResult RenderContext::Submit()
{
    vkCmdEndRenderPass(m_commandBuffers[m_imageIndex]);

    VkResult result = vkEndCommandBuffer(m_commandBuffers[m_imageIndex]);
    LOG_MESSAGE_ASSERT(result == VK_SUCCESS, "Error recording command buffer: " << result);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_imageAvailableSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderFinishedSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_imageIndex];

    return vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, m_fences[m_imageIndex]);
}

VkResult RenderContext::Present()
{
    VkSwapchainKHR swapChains[] = { swapChain.sc };
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderFinishedSemaphore;
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

bool RenderContext::RecreateSwapChain()
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
    CreateFences();
    CreateSemaphores();

    VK_VERIFY(vk::createRenderPass(device, swapChain, &renderPass));
    VK_VERIFY(vk::createCommandPool(device, &commandPool));
    // build the swap chain
    RecreateSwapChain();
    // allocate 2 command buffers (used to be m_frameBuffers.size())
    VK_VERIFY(vk::createCommandBuffers(device, commandPool, m_commandBuffers, NUM_CMDBUFFERS));

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
    m_frameBuffers.resize(m_imageViews.size());

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

        VkResult result = vkCreateFramebuffer(device.logical, &fbCreateInfo, nullptr, &m_frameBuffers[i]);

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
    for (VkFramebuffer &fb : m_frameBuffers)
    {
        vkDestroyFramebuffer(device.logical, fb, nullptr);
    }

    m_frameBuffers.clear();
}

void RenderContext::CreateFences()
{
    VkFenceCreateInfo fCreateInfo = {};
    fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    m_fences.resize(NUM_CMDBUFFERS);

    for (VkFence &fence : m_fences)
    {
        VK_VERIFY(vkCreateFence(device.logical, &fCreateInfo, nullptr, &fence));
    }
}

void RenderContext::CreateSemaphores()
{
    VkSemaphoreCreateInfo sCreateInfo = {};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &m_imageAvailableSemaphore));
    VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &m_renderFinishedSemaphore));
}
