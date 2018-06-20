Quake BSP map viewer in Vulkan
================

This is a BSP tree Vulkan renderer written in C++ and a port of the same viewer [written in OpenGL](https://github.com/kondrak/quake_bsp_viewer_vr). It handles basic geometry and curved patch rendering but with no support for game-specific shaders, entities etc. It implement PVS and frustum culling so performance is optimal. At the moment only Quake III Arena maps are supported but an interface is provided for adding other BSP versions in the future.

![Screenshot](http://kondrak.info/images/qbsp/qbspvk.png?raw=true)

Building
-----
The project should work out of the box with latest Vulkan SDK (tested against SDK 1.0.65.1 and corresponding runtime) and Visual Studio 2015+

Usage
-----
Running the viewer:

<code>QuakeBspViewer.exe &lt;path-to-bsp-file&gt; </code>

Use tilde key (~) to toggle statistics menu on/off. Note that you must have Quake III Arena textures and models unpacked in the root directory if you want to see proper texturing. To move around use the WASD keys. RF keys lift you up/down and QE keys let you do the barrel roll.

Dependencies
-------
This project uses following external libraries:

- [stb_image](https://github.com/nothings/stb) library for image handling (c) Sean Barret
- SDL2 library for window/input
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for painless memory management of Vulkan resources

Vulkan code overview
-------
For people interested in learning Vulkan and jumping into core features, the `renderer/vulkan` folder contains all files related to creating buffers, instances, surfaces and command lists. Note that this is not an attempt at writing an overly abstract rendering engine and few settings are tailored for this particular project. Core BSP and overlay rendering is done in `src/Q3BspMap.cpp` and `src/renderer/Font.cpp`. Global Vulkan handles (instance, device, etc.) are stored in `src/RenderContext.cpp`.

Keyword list:
- multiple pipeline rendering
- immediate screen updates with double buffering and semaphore scheduling
- texture mapping with and without filtering
- 3rd party memory allocation handling using VMA
- validation layers defaulted to debug builds
- Multisampling (MSAA) defaulting to maximum sample count supported by hardware
