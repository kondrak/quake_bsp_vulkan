#include "renderer/RenderContext.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Pipeline.hpp"
#include "renderer/vulkan/Validation.hpp"
#include "renderer/TextureManager.hpp"
#include "Utils.hpp"
#include <algorithm>

// index of the command buffer that's currently in use
static int s_currentCmdBuffer = 0;

// Returns the maximum sample count usable by the platform
static VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties &deviceProperties)
{
    VkSampleCountFlags counts = std::min(deviceProperties.limits.framebufferColorSampleCounts, deviceProperties.limits.framebufferDepthSampleCounts);
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
    return VK_SAMPLE_COUNT_1_BIT;
}

// initialize Vulkan render context
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

    return InitVulkan(title);
}

void RenderContext::Destroy()
{
    if (window)
    {
        vkDeviceWaitIdle(device.logical);

        vk::destroyRenderPass(device, renderPass);
        vk::freeCommandBuffers(device, device.commandPool, m_commandBuffers);
        vkDestroyCommandPool(device.logical, device.commandPool, nullptr);
        vkDestroyCommandPool(device.logical, device.transferCommandPool, nullptr);
 
        DestroyFramebuffers();
        DestroyImageViews();
        DestroyDrawBuffers();

        TextureManager::GetInstance()->ReleaseTextures();

        vkDestroySwapchainKHR(device.logical, swapChain.sc, nullptr);

        for (int i = 0; i < NUM_CMDBUFFERS; ++i)
        {
            vkDestroySemaphore(device.logical, m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device.logical, m_renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device.logical, m_fences[i], nullptr);
        }

        vk::destroyAllocator(device.allocator);
        vkDestroyPipelineCache(device.logical, pipelineCache, nullptr);
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
    VkResult result = vkAcquireNextImageKHR(device.logical, swapChain.sc, UINT64_MAX, m_imageAvailableSemaphores[s_currentCmdBuffer], VK_NULL_HANDLE, &m_imageIndex);
    activeCmdBuffer = m_commandBuffers[s_currentCmdBuffer];
 
    // swapchain has become incompatible - need to recreate it
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        LOG_MESSAGE("SwapChain incompatible after vkAcquireNextImageKHR - rebuilding!");
        RecreateSwapChain();
        return result;
    }

    VK_VERIFY(vkWaitForFences(device.logical, 1, &m_fences[s_currentCmdBuffer], VK_TRUE, UINT64_MAX));
    vkResetFences(device.logical, 1, &m_fences[s_currentCmdBuffer]);

    LOG_MESSAGE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Could not acquire swapchain image: " << result);

    // setup command buffers and render pass for drawing
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    result = vkBeginCommandBuffer(m_commandBuffers[s_currentCmdBuffer], &beginInfo);
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

    vkCmdBeginRenderPass(m_commandBuffers[s_currentCmdBuffer], &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(m_commandBuffers[s_currentCmdBuffer], 0, 1, &m_viewport);
    vkCmdSetScissor(m_commandBuffers[s_currentCmdBuffer], 0, 1, &m_scissor);

    return VK_SUCCESS;
}

VkResult RenderContext::Submit()
{
    vkCmdEndRenderPass(m_commandBuffers[s_currentCmdBuffer]);

    VkResult result = vkEndCommandBuffer(m_commandBuffers[s_currentCmdBuffer]);
    LOG_MESSAGE_ASSERT(result == VK_SUCCESS, "Error recording command buffer: " << result);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[s_currentCmdBuffer];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[s_currentCmdBuffer];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[s_currentCmdBuffer];

    return vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, m_fences[s_currentCmdBuffer]);
}

VkResult RenderContext::Present()
{
    VkSwapchainKHR swapChains[] = { swapChain.sc };
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[s_currentCmdBuffer];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_imageIndex;
    presentInfo.pResults = nullptr;

    VkResult renderResult = vkQueuePresentKHR(device.presentQueue, &presentInfo);

    // recreate swapchain if it's out of date
    if (renderResult == VK_ERROR_OUT_OF_DATE_KHR || renderResult == VK_SUBOPTIMAL_KHR)
    {
        LOG_MESSAGE("SwapChain out of date/suboptimal after vkQueuePresentKHR - rebuilding!");
        RecreateSwapChain();
    }

    s_currentCmdBuffer = (s_currentCmdBuffer + 1) % NUM_CMDBUFFERS;

    return renderResult;
}

Math::Vector2f RenderContext::WindowSize()
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical, m_surface, &surfaceCaps);
    LOG_MESSAGE_ASSERT(surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max(), "WM sets extent width and height to max uint32!");

    // fallback if WM sets extent dimensions to max uint32
    if (surfaceCaps.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
        // fetch current window width and height from SDL, since we can't rely on WM in this case
        SDL_GetWindowSize(window, &width, &height);
        float w = (float)std::max(surfaceCaps.minImageExtent.width,  std::min(surfaceCaps.maxImageExtent.width,  (uint32_t)width));
        float h = (float)std::max(surfaceCaps.minImageExtent.height, std::min(surfaceCaps.maxImageExtent.height, (uint32_t)height));
        return Math::Vector2f(w, h);
    }

    return Math::Vector2f((float)surfaceCaps.currentExtent.width, (float)surfaceCaps.currentExtent.height);
}

// would be nicer to use a separate render pass instead of recreating the existing one!
VkSampleCountFlagBits RenderContext::ToggleMSAA()
{
    vkDeviceWaitIdle(device.logical);
    vk::destroyRenderPass(device, renderPass);
    m_msaaSamples = (m_msaaSamples == VK_SAMPLE_COUNT_1_BIT) ? getMaxUsableSampleCount(device.properties) : VK_SAMPLE_COUNT_1_BIT;
    renderPass.sampleCount = m_msaaSamples;
    VK_VERIFY(vk::createRenderPass(device, swapChain, &renderPass));
    RecreateSwapChain();

    return m_msaaSamples;
}

bool RenderContext::RecreateSwapChain()
{
    vkDeviceWaitIdle(device.logical);
    DestroyFramebuffers();
    DestroyImageViews();

    // set initial swap chain extent to current window size - in case WM can't determine it by itself
    swapChain.extent = { (uint32_t)width, (uint32_t)height };
    VK_VERIFY(vk::createSwapChain(device, m_surface, &swapChain, swapChain.sc));

    m_viewport.width  = (float)swapChain.extent.width;
    m_viewport.height = (float)swapChain.extent.height;
    m_scissor.extent = swapChain.extent;

    // update internal render context dimensions
    width  = swapChain.extent.width;
    height = swapChain.extent.height;
    halfWidth  = width >> 1;
    halfHeight = height >> 1;
    scrRatio = m_viewport.width / m_viewport.height;
    left = -scrRatio;
    right = scrRatio;

    DestroyDrawBuffers();
    CreateDrawBuffers();
    if (!CreateImageViews()) return false;
    if (!CreateFramebuffers()) return false;

    return true;
}

bool RenderContext::InitVulkan(const char *appTitle)
{
    VK_VERIFY(vk::createInstance(window, &m_instance, appTitle));
    // "oldschool" way of creating a Vulkan surface in SDL prior to 2.0.6
    //VK_VERIFY(vk::createSurface(window, m_instance, &m_surface));
    SDL_Vulkan_CreateSurface(window, m_instance, &m_surface);

    device = vk::createDevice(m_instance, m_surface);
    VK_VERIFY(vk::createAllocator(device, &device.allocator));
    // set initial swap chain extent to current window size - in case WM can't determine it by itself
    swapChain.extent = { (uint32_t)width, (uint32_t)height };
    // desired present mode
    swapChain.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    VK_VERIFY(vk::createSwapChain(device, m_surface, &swapChain, VK_NULL_HANDLE));

    m_viewport.x = 0.f;
    m_viewport.y = 0.f;
    m_viewport.minDepth = 0.f;
    m_viewport.maxDepth = 1.f;
    m_viewport.width  = (float)swapChain.extent.width;
    m_viewport.height = (float)swapChain.extent.height;

    m_scissor.offset.x = 0;
    m_scissor.offset.y = 0;
    m_scissor.extent = swapChain.extent;

    CreateFences();
    CreateSemaphores();
    CreatePipelineCache();

    VK_VERIFY(vk::createRenderPass(device, swapChain, &renderPass));
    VK_VERIFY(vk::createCommandPool(device, device.graphicsFamilyIndex, &device.commandPool));
    VK_VERIFY(vk::createCommandPool(device, device.transferFamilyIndex, &device.transferCommandPool));
    CreateDrawBuffers();
    if (!CreateImageViews()) return false;
    if (!CreateFramebuffers()) return false;

    // allocate NUM_CMDBUFFERS command buffers (used to be m_frameBuffers.size())
    VK_VERIFY(vk::createCommandBuffers(device, device.commandPool, m_commandBuffers, NUM_CMDBUFFERS));

    return true;
}

void RenderContext::CreateDrawBuffers()
{
    // standard depth buffer
    m_depthBuffer.resize(swapChain.images.size());

    for (size_t i = 0; i < swapChain.images.size(); ++i)
    {
        m_depthBuffer[i] = vk::createDepthBuffer(device, swapChain, m_msaaSamples);
    }

    // additional render targets for MSAA (if enabled)
    m_msaaColor.resize(swapChain.images.size());

    if (m_msaaSamples != VK_SAMPLE_COUNT_1_BIT)
        CreateMSAABuffers();
}

void RenderContext::DestroyDrawBuffers()
{
    for (vk::Texture &db : m_depthBuffer)
    {
        if (db.image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(device.allocator, db.image, db.allocation);
            vkDestroyImageView(device.logical, db.imageView, nullptr);
            db.image = VK_NULL_HANDLE;
            db.imageView = VK_NULL_HANDLE;
        }
    }

    m_depthBuffer.clear();

    DestroyMSAABuffers();
}

void RenderContext::CreateMSAABuffers()
{
    for (size_t i = 0; i < swapChain.images.size(); ++i)
    {
        m_msaaColor[i] = vk::createColorBuffer(device, swapChain, m_msaaSamples);
    }
}

void RenderContext::DestroyMSAABuffers()
{
    for (size_t i = 0; i < swapChain.images.size(); ++i)
    {
        if (m_msaaColor[i].image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(device.allocator, m_msaaColor[i].image, m_msaaColor[i].allocation);
            vkDestroyImageView(device.logical, m_msaaColor[i].imageView, nullptr);
            m_msaaColor[i].image = VK_NULL_HANDLE;
            m_msaaColor[i].imageView = VK_NULL_HANDLE;
        }
    }

    m_msaaColor.clear();
}

bool RenderContext::CreateImageViews()
{
    m_imageViews.resize(swapChain.images.size());

    for (size_t i = 0; i < swapChain.images.size(); ++i)
    {
        VkResult result = vk::createImageView(device, swapChain.images[i], VK_IMAGE_ASPECT_COLOR_BIT, &m_imageViews[i], swapChain.format, 1);

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

bool RenderContext::CreateFramebuffers()
{
    m_frameBuffers.resize(m_imageViews.size());

    VkFramebufferCreateInfo fbCreateInfo = {};
    fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.renderPass = renderPass.renderPass;
    fbCreateInfo.attachmentCount = (m_msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? 3 : 2;
    fbCreateInfo.width  = swapChain.extent.width;
    fbCreateInfo.height = swapChain.extent.height;
    fbCreateInfo.layers = 1;

    for (size_t i = 0; i < m_frameBuffers.size(); ++i)
    {
        VkImageView attachments[] = { m_imageViews[i], m_depthBuffer[i].imageView };
        VkImageView attachmentsMSAA[] = { m_msaaColor[i].imageView, m_depthBuffer[i].imageView, m_imageViews[i] };

        fbCreateInfo.pAttachments = (m_msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? attachmentsMSAA : attachments;
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

    for (int i = 0; i < NUM_CMDBUFFERS; ++i)
    {
        VK_VERIFY(vkCreateFence(device.logical, &fCreateInfo, nullptr, &m_fences[i]));
    }
}

void RenderContext::CreateSemaphores()
{
    VkSemaphoreCreateInfo sCreateInfo = {};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < NUM_CMDBUFFERS; ++i)
    {
        VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &m_imageAvailableSemaphores[i]));
        VK_VERIFY(vkCreateSemaphore(device.logical, &sCreateInfo, nullptr, &m_renderFinishedSemaphores[i]));
    }
}

void RenderContext::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo pcInfo = {};
    pcInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VK_VERIFY(vkCreatePipelineCache(device.logical, &pcInfo, nullptr, &pipelineCache));
}
