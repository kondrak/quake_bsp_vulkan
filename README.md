Quake BSP map viewer in Vulkan
================

This is a multithreaded BSP tree Vulkan renderer written in C++ and a port of the same viewer [written in OpenGL](https://github.com/kondrak/quake_bsp_viewer_vr). It handles basic geometry and curved patch rendering but with no support for game-specific shaders, entities etc. It implement PVS and frustum culling so performance is optimal. At the moment only Quake III Arena maps are supported but an interface is provided for adding other BSP versions in the future.

![Screenshot](http://kondrak.info/images/qbsp/qbspvk.png?raw=true)

Building
-----
The project should work out of the box with latest Vulkan SDK (tested against SDK 1.1.82.0) and Visual Studio 2015+

Building on Linux and MacOS
-----
Assuming that the Vulkan SDK is already downloaded and properly set up on your target platform:
- set the VULKAN_SDK environment variable pointing to the location of downloaded Vulkan SDK
- download and install SDL2 (`libsdl2-dev` 2.0.7 or higher for Linux, `SDL2.framework` 2.0.8 or higher for MacOS)
- run the Makefile (Linux) or XCode project (MacOS) to build the application

Usage
-----
Running the viewer:

<code>QuakeBspViewer.exe &lt;path-to-bsp-file&gt; </code>

Running the viewer with multithreaded renderer:

<code>QuakeBspViewer.exe &lt;path-to-bsp-file&gt; -mt </code>

Use tilde key (~) to toggle statistics menu on/off. Note that you must have Quake III Arena textures and models unpacked in the root directory if you want to see proper texturing. To move around use the WASD keys. RF keys lift you up/down and QE keys let you do the barrel roll.

OpenGL vs Vulkan
----------------
Performance comparison between OpenGL, Vulkan and Vulkan multithreaded versions (tested on Intel i5 and GTX 970). Differences become more apparent as the amount of rendered geometry increases:

|           |   PVS + Frustum   |  full BSP render  |
|-----------|:-----------------:|------------------:|
|  OpenGL   | 0.98ms (1025 FPS) | 6.89ms (145 FPS)  |
|  Vulkan   | 0.37ms (2615 FPS) | 1.01ms (983 FPS)  |
| Vulkan MT | 0.38ms (2635 FPS) | 0.69ms (1450 FPS) |


Dependencies
-------
This project uses following external libraries:

- [stb_image](https://github.com/nothings/stb) library for texture loading
- SDL2 library for window/input
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for painless memory management of Vulkan resources

Vulkan code overview
-------
For people interested in learning Vulkan and jumping into core features, the `renderer/vulkan` folder contains all files related to creating buffers, instances, surfaces and command lists. Note that this is not an attempt at writing an overly abstract rendering engine and few settings are tailored for this particular project. Core BSP and overlay rendering is done in `src/Q3BspMap.cpp` and `src/renderer/Font.cpp`. Global Vulkan handles (instance, device, etc.) are stored in `src/RenderContext.cpp`.

Keyword list:
- multiple pipeline rendering
- multithreaded command buffer generation (secondary buffers for threads and one primary presentation buffer)
- pipeline dynamic state, derivatives and cache
- texture mapping (filtered and unfiltered)
- memory allocation handled using VMA
- validation layers enabled by default in debug builds
- multisampling (MSAA) using maximum sample count supported by hardware
- mipmapping

Vulkan references
-------
- https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html
- https://arm-software.github.io/vulkan-sdk
- https://www.khronos.org/assets/uploads/developers/library/2016-vulkan-devday-uk/7-Keeping-your-GPU-fed.pdf
- https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
- https://developer.nvidia.com/sites/default/files/akamai/gameworks/blog/munich/mschott_vulkan_multi_threading.pdf
