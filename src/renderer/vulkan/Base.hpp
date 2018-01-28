#pragma once
struct SDL_Window;

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #include <vulkan/vulkan.h>
    #include <Windows.h>
#else
    #error "Only Windows platform supported for now."
#endif

#ifndef NDEBUG
    #define VALIDATION_LAYERS_ON
#endif

// fetch and call Vulkan extension function
#define callVkF(func, inst, ...) { PFN_##func fptr = (PFN_##func)vkGetInstanceProcAddr(inst, #func); \
                                   LOG_MESSAGE_ASSERT(inst, "Attempting lookup with empty instance!"); \
                                   LOG_MESSAGE_ASSERT(fptr, "Function does not exist: " #func); \
                                   (void)fptr(instance, __VA_ARGS__); \
}

#include "Utils.hpp"
#define VK_VERIFY(r) \
    if(r != VK_SUCCESS) { \
        std::stringstream msgStr; \
        msgStr << "Invalid VkResult: " << r << " in " << __FILE__ << ":" << __LINE__ << "\n"; \
        LogError(msgStr.str().c_str()); \
        __debugbreak(); \
    }

#include "renderer/vulkan/vk_mem_alloc.h"

namespace vk
{
    struct Device
    {
        VkPhysicalDevice physical = VK_NULL_HANDLE;
        VkDevice logical = VK_NULL_HANDLE;
        VmaAllocator allocator = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue  = VK_NULL_HANDLE;
        int queueFamilyIndex   = -1; // physical device queue family index
        int presentFamilyIndex = -1; // physical device presentation family index
    };

    struct Descriptor
    {
        VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet set   = VK_NULL_HANDLE; // automatically cleaned with descriptor pools
    };

    VkResult createInstance(SDL_Window *window, VkInstance *instance, const char *title);
    //VkResult createSurface(const void *window, const VkInstance &instance, VkSurfaceKHR *surface);
}
