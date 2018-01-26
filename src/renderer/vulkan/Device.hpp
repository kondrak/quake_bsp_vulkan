#pragma once
#include "renderer/vulkan/Base.hpp"
#include <vector>

namespace vk
{
    struct SwapChain
    {
        VkSwapchainKHR sc = VK_NULL_HANDLE;
        VkFormat scFormat;
        VkExtent2D scExtent;
        std::vector<VkImage> scImages;
    };

    Device createDevice(const VkInstance &instance, const VkSurfaceKHR &surface);
    SwapChain createSwapChain(const Device &device, const VkSurfaceKHR &surface, const VkSwapchainKHR &oldSwapchain);
}