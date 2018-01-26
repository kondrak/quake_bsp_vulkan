#include "renderer/vulkan/Base.hpp"
#include "renderer/vulkan/Validation.hpp"
#include "Utils.hpp"
#include <SDL.h>
#include <SDL_syswm.h>

namespace vk
{
    VkResult createInstance(VkInstance *instance, const char *title)
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = title;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "custom";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // require only surface extension in this case
        int extCount = 2;
        const char *extensionNames[] = { VK_KHR_SURFACE_EXTENSION_NAME
            , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#ifdef VALIDATION_LAYERS_ON
            , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
        };
        extCount++;
#else
        };
#endif
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extCount;
        createInfo.ppEnabledExtensionNames = extensionNames;

#ifdef VALIDATION_LAYERS_ON
        if (!validationLayersAvailable(validationLayers, 1))
            return VK_RESULT_MAX_ENUM;

        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;
#else
        createInfo.enabledLayerCount = 0;
#endif
        VkResult instanceCreated = vkCreateInstance(&createInfo, nullptr, instance);

#ifdef VALIDATION_LAYERS_ON
        if (instanceCreated == VK_SUCCESS)
        {
            vk::createValidationLayers(*instance);
        }
#endif
        return instanceCreated;
    }

    VkResult createSurface(const void *window, const VkInstance &instance, VkSurfaceKHR *surface)
    {
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        if (!SDL_GetWindowWMInfo((SDL_Window *)window, &wminfo))
        {
            LOG_MESSAGE_ASSERT(false, "Could not get WM info from SDL!");
            return VK_RESULT_MAX_ENUM;
        }

        switch (wminfo.subsystem)
        {
        default:
            LOG_MESSAGE_ASSERT(false, "Unsupported window subsystem: " << wminfo.subsystem);
            return VK_RESULT_MAX_ENUM;
        case SDL_SYSWM_WINDOWS:
            VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
            surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceCreateInfo.hwnd = wminfo.info.win.window;
            surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

            return vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface);
        }
    }
}
