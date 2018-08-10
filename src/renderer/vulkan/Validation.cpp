#include "renderer/vulkan/Validation.hpp"
#include "Utils.hpp"

static bool s_usingEXTDebugUtils = true;
static VkDebugUtilsMessengerEXT validationMessenger;
static VkDebugReportCallbackEXT validationLayerCallback;

// EXT_debug_utils
#define vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger) callVkF(vkCreateDebugUtilsMessengerEXT, instance, pCreateInfo, pAllocator, pMessenger)
#define vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator) callVkF(vkDestroyDebugUtilsMessengerEXT, instance, messenger, pAllocator)
// EXT_debug_report
#define vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback) callVkF(vkCreateDebugReportCallbackEXT, instance, pCreateInfo, pAllocator, pCallback)
#define vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator) callVkF(vkDestroyDebugReportCallbackEXT, instance, callback, pAllocator)

// validation layer callback function (VK_EXT_debug_utils)
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackUtils(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT msgType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                                                         void* userData)
{
    auto typeToStr = [](VkDebugUtilsMessageTypeFlagsEXT &type) -> const char* {
        bool g = (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0;
        bool p = (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0;
        bool v = (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0;

        if (g) return "GENERAL";
        if (p && !v) return "PERFORMANCE";
        if (p &&  v) return "PERFORMANCE AND VALIDATION";
        if (v) return "VALIDATION";

        return "<unknown>";
    };

    switch (msgSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        LOG_MESSAGE("VULKAN " << typeToStr(msgType) << " INFO: " << callbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        LOG_MESSAGE("VULKAN " << typeToStr(msgType) << " VERBOSE: " << callbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        LOG_MESSAGE_ASSERT(false, "VULKAN " << typeToStr(msgType) << " WARNING: " << callbackData->pMessage);
        break;
    default:
        LOG_MESSAGE_ASSERT(false, "VULKAN " << typeToStr(msgType) << " ERROR: " << callbackData->pMessage);
    }
    return VK_FALSE;
}

// validation layer callback function (VK_EXT_debug_report)
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackReport(VkDebugReportFlagsEXT flags,
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
    void createValidationLayers(const VkInstance &instance, bool useEXTDebugUtils)
    {
        LOG_MESSAGE_ASSERT(instance != VK_NULL_HANDLE, "Invalid instance!");
        s_usingEXTDebugUtils = useEXTDebugUtils;

        // some devices may not support EXT_debug_utils
        if (useEXTDebugUtils)
        {
            VkDebugUtilsMessengerCreateInfoEXT callbackInfo = {};
            callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            callbackInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            callbackInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            callbackInfo.pfnUserCallback = debugCallbackUtils;

            VK_VERIFY(vkCreateDebugUtilsMessengerEXT(instance, &callbackInfo, nullptr, &validationMessenger));
        }
        else
        {
            VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
            callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                                 VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            callbackInfo.pfnCallback = debugCallbackReport;

            VK_VERIFY(vkCreateDebugReportCallbackEXT(instance, &callbackInfo, nullptr, &validationLayerCallback));
        }
    }

    void destroyValidationLayers(const VkInstance &instance)
    {
        LOG_MESSAGE_ASSERT(instance != VK_NULL_HANDLE, "Invalid instance!");
        if (s_usingEXTDebugUtils)
        {
            vkDestroyDebugUtilsMessengerEXT(instance, validationMessenger, nullptr);
        }
        else
        {
            vkDestroyDebugReportCallbackEXT(instance, validationLayerCallback, nullptr);
        }
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

