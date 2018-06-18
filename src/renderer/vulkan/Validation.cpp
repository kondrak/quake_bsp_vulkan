#include "renderer/vulkan/Validation.hpp"
#include "Utils.hpp"

static VkDebugReportCallbackEXT validationLayerCallback;

// desired extension functions
#define vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback) callVkF(vkCreateDebugReportCallbackEXT, instance, pCreateInfo, pAllocator, pCallback)
#define vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator) callVkF(vkDestroyDebugReportCallbackEXT, instance, callback, pAllocator)

// validation layer callback function
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
                                                    VkDebugReportObjectTypeEXT objType,
                                                    uint64_t obj, size_t location, int32_t code,
                                                    const char* layerPrefix, const char* msg,
                                                    void* userData)
{
    switch (flags)
    {
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
        LOG_MESSAGE("VULKAN INFO: " << msg);
        break;
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        LOG_MESSAGE("VULKAN DEBUG: " << msg);
        break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT:
        LOG_MESSAGE_ASSERT(false, "VULKAN WARNING: " << msg);
        break;
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
        LOG_MESSAGE_ASSERT(false, "VULKAN PERFORMANCE: " << msg);
        break;
    default:
        LOG_MESSAGE_ASSERT(false, "VULKAN ERROR: " << msg);
    }
    return VK_FALSE;
}

namespace vk
{
    void createValidationLayers(const VkInstance &instance)
    {
        LOG_MESSAGE_ASSERT(instance != VK_NULL_HANDLE, "Invalid instance!");

        VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
        callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | 
                             VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        callbackInfo.pfnCallback = debugCallback;

        vkCreateDebugReportCallbackEXT(instance, &callbackInfo, nullptr, &validationLayerCallback);
    }

    void destroyValidationLayers(const VkInstance &instance)
    {
        LOG_MESSAGE_ASSERT(instance != VK_NULL_HANDLE, "Invalid instance!");
        vkDestroyDebugReportCallbackEXT(instance, validationLayerCallback, nullptr);
    }

    bool validationLayersAvailable(const char **requested, size_t count)
    {
        VkLayerProperties *availableLayers;
        uint32_t layerCount = 0;

        VK_VERIFY(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
        availableLayers = new VkLayerProperties[layerCount];
        VK_VERIFY(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers));

        for (size_t i = 0; i < count; ++i)
        {
            bool layerFound = false;
            for (uint32_t j = 0; j < layerCount; ++j)
            {
                if (!strcmp(requested[i], availableLayers[j].layerName))
                {
                    layerFound = true;
                    break;
                }
            }

            LOG_MESSAGE_ASSERT(layerFound, "Requested validation layer unavailable: " << requested[i]);
            if (!layerFound)
            {
                delete[] availableLayers;
                return false;
            }
        }

        delete[] availableLayers;
        return true;
    }
}

