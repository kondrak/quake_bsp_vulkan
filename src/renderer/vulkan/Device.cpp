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

        vkGetDeviceQueue(device.logical, device.graphicsFamilyIndex, 0, &device.graphicsQueue);
        vkGetDeviceQueue(device.logical, device.presentFamilyIndex, 0, &device.presentQueue);
        vkGetDeviceQueue(device.logical, device.transferFamilyIndex, 0, &device.transferQueue);

        return device;
    }

    VkResult createSwapChain(const Device &device, const VkSurfaceKHR &surface, SwapChain *swapChain, VkSwapchainKHR oldSwapchain)
    {
        SwapChainInfo scInfo = {};
        VkSurfaceFormatKHR surfaceFormat = {};
        VkExtent2D extent = {};
        VkExtent2D currentSize = swapChain->extent;
        getSwapChainInfo(device.physical, surface, &scInfo);
        getSwapSurfaceFormat(scInfo, &surfaceFormat);
        getSwapPresentMode(scInfo, &swapChain->presentMode);
        getSwapExtent(scInfo, &extent, currentSize);

        uint32_t imageCount = scInfo.surfaceCaps.minImageCount;

        // request additional image for triple buffering if using MAILBOX
        if (swapChain->presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            imageCount++;

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

        if (device.presentFamilyIndex != device.graphicsFamilyIndex)
        {
            uint32_t queueFamilyIndices[] = { (uint32_t)device.graphicsFamilyIndex, (uint32_t)device.presentFamilyIndex };
            scCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            scCreateInfo.queueFamilyIndexCount = 2;
            scCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        scCreateInfo.preTransform = scInfo.surfaceCaps.currentTransform;
        scCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scCreateInfo.presentMode = swapChain->presentMode;
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
        // at least one queue (graphics and present combined) has to be present
        uint32_t numQueues = 1;
        float queuePriority = 1.f;
        VkDeviceQueueCreateInfo queueCreateInfo[3] = { {}, {}, {} };
        queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[0].queueFamilyIndex = device->graphicsFamilyIndex;
        queueCreateInfo[0].queueCount = 1;
        queueCreateInfo[0].pQueuePriorities = &queuePriority;
        queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[1].queueCount = 1;
        queueCreateInfo[1].pQueuePriorities = &queuePriority;
        queueCreateInfo[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[2].queueCount = 1;
        queueCreateInfo[2].pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures wantedDeviceFeatures = {};
        wantedDeviceFeatures.samplerAnisotropy = device->features.samplerAnisotropy;
        wantedDeviceFeatures.fillModeNonSolid  = device->features.fillModeNonSolid;  // for wireframe rendering
        wantedDeviceFeatures.sampleRateShading = device->features.sampleRateShading; // for sample shading

        // a graphics and present queue are different - two queues have to be created
        if (device->graphicsFamilyIndex != device->presentFamilyIndex)
        {
            queueCreateInfo[numQueues++].queueFamilyIndex = device->presentFamilyIndex;
        }

        // a separate transfer queue exists that's different from present and graphics queue?
        if (device->transferFamilyIndex != device->graphicsFamilyIndex && device->transferFamilyIndex != device->presentFamilyIndex)
        {
            queueCreateInfo[numQueues++].queueFamilyIndex = device->transferFamilyIndex;
        }

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pEnabledFeatures = &wantedDeviceFeatures;
        deviceCreateInfo.ppEnabledExtensionNames = devExtensions.data();
        deviceCreateInfo.enabledExtensionCount = (uint32_t)devExtensions.size();
        deviceCreateInfo.queueCreateInfoCount = numQueues;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

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
                        device->graphicsFamilyIndex = j;
                    }

                    if (queueFamilies[j].queueCount > 0 && !(queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamilies[j].queueFlags & VK_QUEUE_TRANSFER_BIT))
                    {
                        device->transferFamilyIndex = j;
                    }
                }

                delete[] queueFamilies;

                // accept only device that has support for presentation and drawing
                if (device->presentFamilyIndex >= 0 && device->graphicsFamilyIndex >= 0)
                {
                    if (device->transferFamilyIndex < 0)
                    {
                        device->transferFamilyIndex = device->graphicsFamilyIndex;
                    }

                    device->physical = devices[i];
                    device->properties = deviceProperties;
                    device->features = deviceFeatures;
                    return;
                }
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
        // check if the desired present mode is supported
        if (*presentMode != VK_PRESENT_MODE_MAX_ENUM_KHR)
        {
            for (uint32_t i = 0; i < scInfo.presentModesCount; ++i)
            {
                // mode supported, nothing to do here
                if (scInfo.presentModes[i] == *presentMode)
                {
                    LOG_MESSAGE("Preferred present mode found: " << *presentMode);
                    return;
                }
            }

            LOG_MESSAGE("Preferred present mode " << *presentMode << " not supported!");
            // preferred mode not supported - mark and find the next best thing
            *presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        }

        // no preferred present mode/preferred not found - choose the next best thing
        if (*presentMode == VK_PRESENT_MODE_MAX_ENUM_KHR)
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

            LOG_MESSAGE("Using present mode: " << *presentMode);
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
