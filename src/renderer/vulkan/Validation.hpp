#pragma once
#include "renderer/vulkan/Base.hpp"

/*
 * Vulkan validation layers
 */

namespace vk
{
#ifdef VALIDATION_LAYERS_ON
    // requested validation layers
#ifndef __ANDROID__
    static const int validationLayerCount = 1;
    static const char *validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
#else
    static const int validationLayerCount = 6;
    static const char *validationLayers[] = { "VK_LAYER_GOOGLE_threading",
                                              "VK_LAYER_LUNARG_parameter_validation",
                                              "VK_LAYER_LUNARG_object_tracker",
                                              "VK_LAYER_LUNARG_core_validation",
                                              "VK_LAYER_LUNARG_swapchain",
                                              "VK_LAYER_GOOGLE_unique_objects"
                                            };
#endif
#endif

    void createValidationLayers(const VkInstance &instance, bool useEXTDebugUtils);
    void destroyValidationLayers(const VkInstance &instance);
    bool validationLayersAvailable(const char **requested, size_t count);
}
