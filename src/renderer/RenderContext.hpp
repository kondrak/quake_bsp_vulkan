#ifndef RENDERCONTEXT_INCLUDED
#define RENDERCONTEXT_INCLUDED

#include "Math.hpp"
#include "renderer/vulkan/Buffers.hpp"
#include "renderer/vulkan/Device.hpp"
#include "renderer/vulkan/Image.hpp"
#include "renderer/vulkan/Pipeline.hpp"
#include <SDL.h>
#include <SDL_syswm.h>
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

    VkResult RenderStart();
    VkResult Submit(const std::vector<VkCommandBuffer> &commandBuffers);
    VkResult Present(bool uiVisible);
    bool RecreateSwapChain(const VkCommandPool &commandPool, const vk::RenderPass &renderPasss);

    SDL_Window *window = nullptr;
    vk::Device device;
    vk::SwapChain swapChain;
    std::vector<VkFramebuffer> frameBuffers;
    VkSubmitInfo submitInfo = {};
    VkSemaphore renderFinishedSemaphore;
    VkSemaphore renderUIFinishedSemaphore;

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
    bool CreateImageViews();
    void DestroyImageViews();
    bool CreateFramebuffers(const vk::RenderPass &renderPass);
    void DestroyFramebuffers();
    void CreateSemaphores();

    // Vulkan instance and surface
    VkInstance   m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface  = VK_NULL_HANDLE;

    // Vulkan image views
    std::vector<VkImageView> m_imageViews;

    // semaphore: signal when next image is available for rendering
    VkSemaphore m_imageAvailableSemaphore;

    // depth buffer texture
    vk::Texture m_depthBuffer;

    // handle submission from multiple render passes
    uint32_t m_imageIndex;
};

#endif