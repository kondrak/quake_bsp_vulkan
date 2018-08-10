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
    bool Init(const char *title, int x, int y, int w, int h);
    void Destroy();

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

    // Vulkan global objects
    vk::Device device;
    vk::SwapChain swapChain;
    vk::RenderPass renderPass;
    VkCommandBuffer activeCmdBuffer = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;

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
private:
    bool InitVulkan(const char *appTitle);
    void CreateDrawBuffers();
    void DestroyDrawBuffers();
    void CreateMSAABuffers();
    void DestroyMSAABuffers();
    bool CreateImageViews();
    void DestroyImageViews();
    bool CreateFramebuffers();
    void DestroyFramebuffers();
    void CreateFences();
    void CreateSemaphores();
    void CreatePipelineCache();

    // Vulkan instance and surface
    VkInstance   m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface  = VK_NULL_HANDLE;

    // using dynamic states for pipelines, so we need to update viewport and scissor manually
    VkViewport m_viewport = {};
    VkRect2D   m_scissor  = {};

    // Vulkan framebuffers
    std::vector<VkFramebuffer> m_frameBuffers;

    // Vulkan image views
    std::vector<VkImageView> m_imageViews;

    // use 2 synchronized command buffers for rendering (double buffering)
    static const int NUM_CMDBUFFERS = 2;

    // command buffers
    std::vector<VkCommandBuffer> m_commandBuffers;
    // command buffer double buffering fences
    VkFence m_fences[NUM_CMDBUFFERS];
    // semaphore: signal when next image is available for rendering
    VkSemaphore m_imageAvailableSemaphores[NUM_CMDBUFFERS];
    // semaphore: signal when rendering to current command buffer is complete
    VkSemaphore m_renderFinishedSemaphores[NUM_CMDBUFFERS];

    // depth buffer textures - one per each swapchain image
    std::vector<vk::Texture> m_depthBuffer;

    // render targets for color and depth used with MSAA (if sample count > 1) - one per each swapchain image
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    std::vector<vk::Texture> m_msaaColor;

    // handle submission from multiple render passes
    uint32_t m_imageIndex;
};

#endif