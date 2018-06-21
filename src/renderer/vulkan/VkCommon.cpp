// common symbols and defines

#ifdef _WIN32
#    define VK_USE_PLATFORM_WIN32_KHR
#endif

// enable validation layers in debug builds by default
#ifndef NDEBUG
#    define VALIDATION_LAYERS_ON
#endif

#define VMA_IMPLEMENTATION
#include <vulkan/vulkan.h>
#undef min
#undef max
#include "renderer/vulkan/vk_mem_alloc.h"
