#pragma once
#include "renderer/vulkan/Base.hpp"

/*
 * Vulkan validation layers
 */

namespace vk
{
#ifdef VALIDATION_LAYERS_ON
    // requested validation layers
    static const char *validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
#endif

    void createValidationLayers(const VkInstance &instance);
    void destroyValidationLayers(const VkInstance &instance);
    bool validationLayersAvailable(const char **requested, size_t count);
}
