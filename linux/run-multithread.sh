#!/bin/bash
LD_LIBRARY_PATH=$VULKAN_SDK/x86_64/lib VK_LAYER_PATH=$VULKAN_SDK/x86_64/etc/explicit_layer.d ./QuakeBspViewer maps/ntkjidm2.bsp -mt
