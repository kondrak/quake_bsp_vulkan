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


// UBO used by the main shader
struct UniformBufferObject
{
    Math::Matrix4f ModelViewProjectionMatrix;
    float worldScaleFactor;
    int renderLightmaps = 0;
    int useLightmaps = 1;
    int useAlphaTest = 0;
};

// GLSL attribute IDs for both the main and font shaders
enum Attributes : uint32_t
{
    inVertex = 0,
    inTexCoord = 1,
    inTexCoordLightmap = 2,
    inColor = 2,
};


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

    SDL_Window *window = nullptr;

    // Vulkan global objects
    vk::Device device;
    vk::SwapChain swapChain;
    vk::RenderPass renderPass;
    VkCommandPool   commandPool = VK_NULL_HANDLE;
    VkCommandBuffer activeCmdBuffer = VK_NULL_HANDLE;

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
    bool InitVulkan();
    void CreateDepthBuffer(const VkCommandPool &commandPool);
    void DestroyDepthBuffer();
    bool CreateImageViews();
    void DestroyImageViews();
    bool CreateFramebuffers(const vk::RenderPass &renderPass);
    void DestroyFramebuffers();
    void CreateFences();
    void CreateSemaphores();

    // Vulkan instance and surface
    VkInstance   m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface  = VK_NULL_HANDLE;

    // Vulkan framebuffers
    std::vector<VkFramebuffer> m_frameBuffers;

    // Vulkan image views
    std::vector<VkImageView> m_imageViews;

    // command buffers
    std::vector<VkCommandBuffer> m_commandBuffers;

    // command buffer double buffering fences
    std::vector<VkFence> m_fences;
    // semaphore: signal when next image is available for rendering
    VkSemaphore m_imageAvailableSemaphore;
    // semaphore: signal when rendering to current command buffer is complete
    VkSemaphore m_renderFinishedSemaphore;

    // depth buffer texture
    vk::Texture m_depthBuffer;

    // handle submission from multiple render passes
    uint32_t m_imageIndex;
};

#endif