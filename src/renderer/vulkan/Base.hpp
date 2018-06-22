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
#define callVkF(func, inst, ...) { PFN_##func fptr = (PFN_##func)vkGetInstanceProcAddr(inst, #func); \
                                   LOG_MESSAGE_ASSERT(inst, "Invalid Vulkan instance!"); \
                                   LOG_MESSAGE_ASSERT(fptr, "Function does not exist: " #func); \
                                   (void)fptr(instance, __VA_ARGS__); \
}

// verify if VkResult is VK_SUCCESS
#include "Utils.hpp"
#define VK_VERIFY(r) \
    if(r != VK_SUCCESS) { \
        std::stringstream msgStr; \
        msgStr << "Invalid VkResult: " << r << " in " << __FILE__ << ":" << __LINE__ << "\n"; \
        LogError(msgStr.str().c_str()); \
        __debugbreak(); \
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

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue  = VK_NULL_HANDLE;
        int queueFamilyIndex   = -1; // physical device queue family index
        int presentFamilyIndex = -1; // physical device presentation family index
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
