#!/bin/bash
LD_LIBRARY_PATH=$VULKAN_SDK/lib VK_LAYER_PATH=$VULKAN_SDK/etc/explicit_layer.d ./QuakeBspViewer maps/ntkjidm2.bsp
