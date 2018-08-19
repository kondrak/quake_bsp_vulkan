#ifndef RENDERCONTEXT_INCLUDED
#define RENDERCONTEXT_INCLUDED

#include "Math.hpp"
#include "renderer/vulkan/Buffers.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Device.hpp"
#include "renderer/vulkan/Image.hpp"
#include "renderer/vulkan/Pipeline.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>

// SDL-based Vulkan setup container ("render context")
class RenderContext
{
public:
    bool Init(const char *title, bool primaryDoubleBuffer, int x, int y, int w, int h);
    void Destroy();
    const char *WindowTitle() const { return m_windowTitle; }

    // start rendering frame and setup all necessary structs
    VkResult RenderStart();
    // command buffer submission to render queue
    VkResult Submit();
    // render queue presentation
    VkResult Present();
    // fetch current renderable surface size directly from Vulkan surface
    Math::Vector2f WindowSize();
    // rebuild entire swap chain
    bool RecreateSwapChain();
    // toggle MSAA on/off, return current setting
    VkSampleCountFlagBits ToggleMSAA();

    SDL_Window *window = nullptr;

    float fov = 75.f * PIdiv180;
    float nearPlane = 0.1f;
    float farPlane  = 1000.f;
    float scrRatio  = 0.f;
    int   width     = 0;
    int   height    = 0;
    int   halfWidth = 0;
    int   halfHeight = 0;

    // ortho parameters
    float left   = 0.f;
    float right  = 0.f;
    float bottom = 0.f;
    float top = 0.f;

    Math::Matrix4f ModelViewProjectionMatrix; // global MVP used to orient the entire world

    // const accessors to Vulkan objects required by application
    const vk::Device &Device()   const { return m_device; }
    const vk::SwapChain &SwapChain() const { return m_swapChain; }
    const VkPipelineCache &PipelineCache() const { return m_pipelineCache; }
    const VkViewport &Viewport() const { return m_viewport; }
    const VkRect2D &Scissor()    const { return m_scissor; }
    const vk::RenderPass &ActiveRenderPass() const { return m_activeRenderPass; }
    const VkFramebuffer &ActiveFramebuffer() const { return m_activeFramebuffer; }
    const VkCommandBuffer &ActiveCmdBuffer() const { return m_activeCmdBuffer; }
private:
    bool InitVulkan(const char *appTitle);
    void CreateDrawBuffers();
    void DestroyDrawBuffers();
    bool CreateImageViews();
    void DestroyImageViews();
    std::vector<VkFramebuffer> CreateFramebuffers(const vk::RenderPass &rp);
    void DestroyFramebuffers();
    void CreateFences();
    void CreateSemaphores();
    void CreatePipelineCache();

    const char *m_windowTitle;

    // Vulkan instance and surface
    VkInstance   m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface  = VK_NULL_HANDLE;

    // Vulkan global objects
    vk::Device m_device;
    vk::SwapChain m_swapChain;
    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;

    // using dynamic states for pipelines, so we need to update viewport and scissor manually
    VkViewport m_viewport = {};
    VkRect2D   m_scissor = {};

    // Vulkan render passes - we don't want to rebuild them mid-flight when toggling MSAA due to issues in full screen (black screen, blinking, etc.)
    vk::RenderPass m_renderPass;
    vk::RenderPass m_msaaRenderPass;

    vk::RenderPass  m_activeRenderPass;
    VkFramebuffer   m_activeFramebuffer = VK_NULL_HANDLE;
    VkCommandBuffer m_activeCmdBuffer = VK_NULL_HANDLE;

    // Vulkan framebuffers
    std::vector<VkFramebuffer> m_frameBuffers;
    std::vector<VkFramebuffer> m_msaaFrameBuffers;

    // Vulkan image views
    std::vector<VkImageView> m_imageViews;

    // command buffers
    std::vector<VkCommandBuffer> m_commandBuffers;
    // command buffer fences
    std::vector<VkFence> m_fences;
    // semaphore: signal when next image is available for rendering
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    // semaphore: signal when rendering to current command buffer is complete
    std::vector<VkSemaphore> m_renderFinishedSemaphores;

    // depth buffer
    vk::Texture m_depthBuffer;

    // render targets for color and depth used with MSAA
    vk::Texture m_msaaColor;
    vk::Texture m_msaaDepthBuffer;

    // handle submission from multiple render passes
    uint32_t m_imageIndex;
};

#endif