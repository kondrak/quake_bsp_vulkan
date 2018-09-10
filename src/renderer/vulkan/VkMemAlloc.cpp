#define VMA_IMPLEMENTATION
#ifdef __ANDROID__
#include "android/vulkan_wrapper.h"
#else
#include <vulkan/vulkan.h>
#endif
#include "renderer/vulkan/vk_mem_alloc.h"
