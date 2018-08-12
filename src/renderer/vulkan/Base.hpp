#pragma once
struct SDL_Window;

#ifdef _WIN32
#    define VK_USE_PLATFORM_WIN32_KHR
#endif

// enable validation layers in debug builds by default
#ifndef NDEBUG
#    define VALIDATION_LAYERS_ON
#endif

#include <vulkan/vulkan.h>
#include "renderer/vulkan/vk_mem_alloc.h"

// fetch and call Vulkan extension function
#define callVkF(func, inst, ...) ((PFN_##func)vkGetInstanceProcAddr(inst, #func))(inst, __VA_ARGS__)

// verify if VkResult is VK_SUCCESS
#include "Utils.hpp"
#define VK_VERIFY(x) { \
    VkResult res = (x); \
    LOG_MESSAGE_ASSERT(res == VK_SUCCESS, "Invalid VkResult: " << res << " in " << __FILE__ << ":" << __LINE__ << "\n"); \
}

namespace vk
{
    // Vulkan device
    struct Device
    {
        VkPhysicalDevice physical  = VK_NULL_HANDLE;
        VkDevice         logical   = VK_NULL_HANDLE;
        VmaAllocator     allocator = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures   features = {};

        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandPool transferCommandPool = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue  = VK_NULL_HANDLE;
        VkQueue transferQueue = VK_NULL_HANDLE;
        int graphicsFamilyIndex = -1; // physical device queue family index
        int presentFamilyIndex  = -1; // physical device presentation family index
        int transferFamilyIndex = -1;
    };

    // Vulkan descriptor
    struct Descriptor
    {
        VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet  set  = VK_NULL_HANDLE; // automatically cleaned with descriptor pools
    };


    VkResult createInstance(SDL_Window *window, VkInstance *instance, const char *title);
    VkResult createDescriptorSet(const Device &device, Descriptor *descriptor);
    // this application uses VMA for memory management
    VkResult createAllocator(const Device &device, VmaAllocator *allocator);
    void    destroyAllocator(VmaAllocator &allocator);
}
