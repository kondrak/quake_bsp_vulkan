#!/bin/sh

$VULKAN_SDK/x86_64/bin/glslangValidator -V res/Basic.vert -o res/Basic_vert.spv
$VULKAN_SDK/x86_64/bin/glslangValidator -V res/Basic.frag -o res/Basic_frag.spv
$VULKAN_SDK/x86_64/bin/glslangValidator -V res/Font.vert -o res/Font_vert.spv
$VULKAN_SDK/x86_64/bin/glslangValidator -V res/Font.frag -o res/Font_frag.spv
