#include "renderer/vulkan/Base.hpp"
#include "renderer/vulkan/Validation.hpp"
#include "Utils.hpp"
#include <SDL_vulkan.h>

namespace vk
{
    VkResult createInstance(SDL_Window *window, VkInstance *instance, const char *title)
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = title;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "custom";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        unsigned int extCount = 0;
        unsigned int additionalExtCount = 0;
#ifdef VALIDATION_LAYERS_ON
        additionalExtCount++;
#endif
        // get count of required extensions
        SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr);
        const char **extensionNames = new const char*[extCount + additionalExtCount];
        // get names of required extensions
        SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensionNames);

#ifdef VALIDATION_LAYERS_ON
        // add additional extensions to the list - validation layer in this case
        const char *additionalExtensions[] = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
        for (unsigned int i = 0; i < additionalExtCount; ++i)
        {
            extensionNames[extCount + i] = additionalExtensions[i];
        }

        extCount += additionalExtCount;
#endif

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extCount;
        createInfo.ppEnabledExtensionNames = extensionNames;

#ifdef VALIDATION_LAYERS_ON
        if (!validationLayersAvailable(validationLayers, 1))
        {
            LOG_MESSAGE_ASSERT(false, "Validation layers not available!");
            delete[] extensionNames;
            return VK_RESULT_MAX_ENUM;
        }

        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;
#else
        createInfo.enabledLayerCount = 0;
#endif
        VkResult instanceCreated = vkCreateInstance(&createInfo, nullptr, instance);

#ifdef VALIDATION_LAYERS_ON
        VK_VERIFY(instanceCreated);
        vk::createValidationLayers(*instance);
#endif
        delete[] extensionNames;
        return instanceCreated;
    }

    VkResult createDescriptorSet(const Device &device, Descriptor *descriptor)
    {
        VkDescriptorSetLayout layouts[] = { descriptor->setLayout };
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptor->pool;
        allocInfo.pSetLayouts = layouts;
        allocInfo.descriptorSetCount = 1;

        return vkAllocateDescriptorSets(device.logical, &allocInfo, &descriptor->set);
    }

    // create VMA allocator
    VkResult createAllocator(const Device &device, VmaAllocator *allocator)
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = device.physical;
        allocatorInfo.device = device.logical;

        return vmaCreateAllocator(&allocatorInfo, allocator);
    }

    // destroy VMA allocator
    void destroyAllocator(VmaAllocator &allocator)
    {
        vmaDestroyAllocator(allocator);
    }

// deprecated Vulkan surface creation prior to SDL 2.0.6
/*
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
*/
}
