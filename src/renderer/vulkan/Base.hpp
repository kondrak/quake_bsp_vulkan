#pragma once
#ifdef __ANDROID__
#include "android/vulkan_wrapper.h"
#endif
#include <SDL_vulkan.h>
#include "renderer/vulkan/vk_mem_alloc.h"
#include "Utils.hpp"

#ifdef _WIN32
#    define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __ANDROID__
#    define VK_USE_PLATFORM_ANDROID_KHR
#endif

// enable validation layers in debug builds by default (at this point iOS has no support for it in MoltenVK at all)
#if !defined(NDEBUG) && !TARGET_OS_IPHONE
#    define VALIDATION_LAYERS_ON
#endif

// fetch and call Vulkan extension function
#define callVkF(func, inst, ...) ((PFN_##func)vkGetInstanceProcAddr(inst, #func))(inst, __VA_ARGS__)

// fetch and call Vulkan extension function (no instance in arg list)
#define callVkF2(func, inst, ...) ((PFN_##func)vkGetInstanceProcAddr(inst, #func))(__VA_ARGS__)

// verify if VkResult is VK_SUCCESS
#define VK_VERIFY(x) { \
    VkResult res = (x); \
    LOG_MESSAGE_ASSERT(res == VK_SUCCESS, "Invalid VkResult: " << res << " in " << __FILE__ << ":" << __LINE__ << "\n"); \
    (void)res; \
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
    VkFormat getBestDepthFormat(const Device &device);
}
