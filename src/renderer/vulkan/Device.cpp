#include "renderer/vulkan/Device.hpp"
#include "renderer/vulkan/Validation.hpp"
#include "Utils.hpp"
#include <algorithm>

namespace vk
{
    // helper struct to avoid passing tons of args to functions
    struct SwapChainInfo
    {
        VkSurfaceCapabilitiesKHR surfaceCaps = {};
        VkSurfaceFormatKHR *formats = nullptr;
        uint32_t formatCount = 0;
        VkPresentModeKHR *presentModes = nullptr;
        uint32_t presentModesCount = 0;

        ~SwapChainInfo()
        {
            delete[] formats;
            delete[] presentModes;
        }
    };

    // requested device extensions
    static std::vector<const char *> devExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // internal helper functions for device and swapchain creation
    static VkResult selectPhysicalDevice(const VkInstance &instance, const VkSurfaceKHR &surface, Device *device);
    static VkResult createLogicalDevice(Device *device);
    static void getBestPhysicalDevice(const VkPhysicalDevice *devices, size_t count, const VkSurfaceKHR &surface, Device *device);
    static bool deviceExtensionsSupported(const VkPhysicalDevice &device, const char **requested, size_t count);
    static void getSwapChainInfo(const VkPhysicalDevice devices, const VkSurfaceKHR &surface, SwapChainInfo *scInfo);
    static void getSwapSurfaceFormat(const SwapChainInfo &scInfo, VkSurfaceFormatKHR *surfaceFormat);
    static void getSwapPresentMode(const SwapChainInfo &scInfo, VkPresentModeKHR *presentMode);
    static void getSwapExtent(const SwapChainInfo &scInfo, VkExtent2D *swapExtent, const VkExtent2D &currentSize);

    Device createDevice(const VkInstance &instance, const VkSurfaceKHR &surface)
    {
        Device device;
        VK_VERIFY(selectPhysicalDevice(instance, surface, &device));
        VK_VERIFY(createLogicalDevice(&device));

        vkGetDeviceQueue(device.logical, device.queueFamilyIndex, 0, &device.graphicsQueue);
        vkGetDeviceQueue(device.logical, device.presentFamilyIndex, 0, &device.presentQueue);

        return device;
    }

    VkResult createSwapChain(const Device &device, const VkSurfaceKHR &surface, SwapChain *swapChain, VkSwapchainKHR oldSwapchain)
    {
        SwapChainInfo scInfo = {};
        VkSurfaceFormatKHR surfaceFormat = {};
        VkPresentModeKHR presentMode = {};
        VkExtent2D extent = {};
        VkExtent2D currentSize = swapChain->extent;
        getSwapChainInfo(device.physical, surface, &scInfo);
        getSwapSurfaceFormat(scInfo, &surfaceFormat);
        getSwapPresentMode(scInfo, &presentMode);
        getSwapExtent(scInfo, &extent, currentSize);

        // add 1 if going for triple buffering
        uint32_t imageCount = scInfo.surfaceCaps.minImageCount;

        VkSwapchainCreateInfoKHR scCreateInfo = {};
        scCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scCreateInfo.surface = surface;
        scCreateInfo.minImageCount = imageCount;
        scCreateInfo.imageFormat = surfaceFormat.format;
        scCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        scCreateInfo.imageExtent = extent;
        scCreateInfo.imageArrayLayers = 1;
        scCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        scCreateInfo.queueFamilyIndexCount = 0;
        scCreateInfo.pQueueFamilyIndices = nullptr;

        if (device.presentFamilyIndex != device.queueFamilyIndex)
        {
            uint32_t queueFamilyIndices[] = { (uint32_t)device.queueFamilyIndex, (uint32_t)device.presentFamilyIndex };
            scCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            scCreateInfo.queueFamilyIndexCount = 2;
            scCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        scCreateInfo.preTransform = scInfo.surfaceCaps.currentTransform;
        scCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scCreateInfo.presentMode = presentMode;
        scCreateInfo.clipped = VK_TRUE;
        scCreateInfo.oldSwapchain = oldSwapchain;

        VK_VERIFY(vkCreateSwapchainKHR(device.logical, &scCreateInfo, nullptr, &swapChain->sc));
        swapChain->format = surfaceFormat.format;
        swapChain->extent = extent;

        // retrieve swap chain images
        VK_VERIFY(vkGetSwapchainImagesKHR(device.logical, swapChain->sc, &imageCount, nullptr));
        LOG_MESSAGE_ASSERT(imageCount != 0, "No available images in the swap chain?");
        swapChain->images.resize(imageCount);

        VkResult result = vkGetSwapchainImagesKHR(device.logical, swapChain->sc, &imageCount, swapChain->images.data());

        // release old swap chain if it was specified
        if(oldSwapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(device.logical, oldSwapchain, nullptr);

        return result;
    }

    VkResult selectPhysicalDevice(const VkInstance &instance, const VkSurfaceKHR &surface, Device *device)
    {
        uint32_t physicalDeviceCount = 0;
        VK_VERIFY(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));

        if (physicalDeviceCount == 0)
        {
            LOG_MESSAGE_ASSERT(false, "No Vulkan-capable devices found!");
            return VK_RESULT_MAX_ENUM;
        }

        VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
        VK_VERIFY(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

        getBestPhysicalDevice(physicalDevices, physicalDeviceCount, surface, device);
        LOG_MESSAGE_ASSERT(device->physical != VK_NULL_HANDLE, "Could not find a suitable physical device!");

        delete[] physicalDevices;
        return VK_SUCCESS;
    }

    VkResult createLogicalDevice(Device *device)
    {
        LOG_MESSAGE_ASSERT(device->physical != VK_NULL_HANDLE, "Invalid physical device!");
        float queuePriority = 1.f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = device->queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE;  // for wireframe rendering

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.ppEnabledExtensionNames = devExtensions.data();
        deviceCreateInfo.enabledExtensionCount = (uint32_t)devExtensions.size();

        // a single queue can draw and present? Provide single create info, otherwise create two separate queues
        if (device->queueFamilyIndex == device->presentFamilyIndex)
        {
            deviceCreateInfo.queueCreateInfoCount = 1;
            deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        }
        else
        {
            VkDeviceQueueCreateInfo queues[] = { queueCreateInfo, queueCreateInfo };
            deviceCreateInfo.queueCreateInfoCount = 2;
            deviceCreateInfo.pQueueCreateInfos = queues;
        }
#ifdef VALIDATION_LAYERS_ON
        deviceCreateInfo.enabledLayerCount = 1;
        deviceCreateInfo.ppEnabledLayerNames = vk::validationLayers;
#else
        deviceCreateInfo.enabledLayerCount = 0;
#endif
        return vkCreateDevice(device->physical, &deviceCreateInfo, nullptr, &device->logical);
    }

    void getBestPhysicalDevice(const VkPhysicalDevice *devices, size_t count, const VkSurfaceKHR &surface, Device *device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        uint32_t queueFamilyCount = 0;

        for (size_t i = 0; i < count; ++i)
        {
            vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
            vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);

            // prefer discrete GPU but if it's the only one available then don't be picky
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || count == 1)
            {
                LOG_MESSAGE_ASSERT(queueFamilyCount != 0, "No queue families for the device!");

                // check if requested device extensions are present
                bool extSupported = deviceExtensionsSupported(devices[i], devExtensions.data(), devExtensions.size());

                // no required extensions? try next device
                if (!extSupported || !deviceFeatures.samplerAnisotropy || !deviceFeatures.fillModeNonSolid)
                    continue;

                // if extensions are fine, query swap chain details and see if we can use this device
                SwapChainInfo scInfo = {};
                getSwapChainInfo(devices[i], surface, &scInfo);

                if (scInfo.formatCount == 0 || scInfo.presentModesCount == 0)
                    continue;

                VkQueueFamilyProperties *queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
                vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);

                // secondary check - device is OK if there's at least on queue with VK_QUEUE_GRAPHICS_BIT set
                for (uint32_t j = 0; j < queueFamilyCount; ++j)
                {
                    // check if this queue family has support for presentation
                    VkBool32 presentSupported;
                    VK_VERIFY(vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, surface, &presentSupported));

                    // good optimization would be to find a queue where presentIdx == queueIdx for less overhead
                    if (queueFamilies[j].queueCount > 0 && presentSupported)
                    {
                        device->presentFamilyIndex = j;
                    }

                    if (queueFamilies[j].queueCount > 0 && queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    {
                        device->queueFamilyIndex = j;
                    }

                    // accept only device that has support for presentation and drawing
                    if (device->presentFamilyIndex >= 0 && device->queueFamilyIndex >= 0)
                    {
                        delete[] queueFamilies;
                        device->physical = devices[i];
                        device->properties = deviceProperties;
                        return;
                    }
                }

                delete[] queueFamilies;
            }
        }
    }

    bool deviceExtensionsSupported(const VkPhysicalDevice &device, const char **requested, size_t count)
    {
        uint32_t extCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);

        if (extCount > 0)
        {
            VkExtensionProperties *extensions = new VkExtensionProperties[extCount];
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, extensions);

            for (size_t i = 0; i < count; ++i)
            {
                bool available = false;
                for (uint32_t j = 0; j < extCount; ++j)
                {
                    available |= !strcmp(extensions[j].extensionName, requested[i]);
                }

                // all requested extensions must be available
                if (!available)
                    return false;
            }
        }

        return true;
    }

    void getSwapChainInfo(VkPhysicalDevice device, const VkSurfaceKHR &surface, SwapChainInfo *scInfo)
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &scInfo->surfaceCaps);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &scInfo->formatCount, nullptr);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &scInfo->presentModesCount, nullptr);

        if (scInfo->formatCount > 0)
        {
            scInfo->formats = new VkSurfaceFormatKHR[scInfo->formatCount];
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &scInfo->formatCount, scInfo->formats);
        }

        if (scInfo->presentModesCount > 0)
        {
            scInfo->presentModes = new VkPresentModeKHR[scInfo->presentModesCount];
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &scInfo->presentModesCount, scInfo->presentModes);
        }
    }

    void getSwapSurfaceFormat(const SwapChainInfo &scInfo, VkSurfaceFormatKHR *surfaceFormat)
    {
        for (size_t i = 0; i < scInfo.formatCount; ++i)
        {
            if (scInfo.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
                scInfo.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                surfaceFormat->colorSpace = scInfo.formats[i].colorSpace;
                surfaceFormat->format = scInfo.formats[i].format;
                return;
            }
        }
        // no preferred format, so get the first one from list
        surfaceFormat->colorSpace = scInfo.formats[0].colorSpace;
        surfaceFormat->format = scInfo.formats[0].format;
    }

    void getSwapPresentMode(const SwapChainInfo &scInfo, VkPresentModeKHR *presentMode)
    {
        *presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (uint32_t i = 0; i < scInfo.presentModesCount; ++i)
        {
            // always prefer mailbox for triple buffering
            if (scInfo.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                *presentMode = scInfo.presentModes[i];
                break;
            }
            else if (scInfo.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                *presentMode = scInfo.presentModes[i];
            }
        }
    }

    void getSwapExtent(const SwapChainInfo &scInfo, VkExtent2D *swapExtent, const VkExtent2D &currentSize)
    {
        if (scInfo.surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            *swapExtent = scInfo.surfaceCaps.currentExtent;
            return;
        }

        // special case when w/h are set to max uint32_t for some WMs - compare against current window size
        swapExtent->width  = std::max(scInfo.surfaceCaps.minImageExtent.width,  std::min(scInfo.surfaceCaps.maxImageExtent.width,  currentSize.width));
        swapExtent->height = std::max(scInfo.surfaceCaps.minImageExtent.height, std::min(scInfo.surfaceCaps.maxImageExtent.height, currentSize.height));
        LOG_MESSAGE("WM sets extent width and height to max uint32!");
    }
}
