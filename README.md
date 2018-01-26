Quake BSP map viewer in Vulkan
================

This is a BSP tree Vulkan renderer written in C++ and a port of the same viewer [written in OpenGL](https://github.com/kondrak/quake_bsp_viewer_vr). It handles basic geometry and curved patch rendering but with no support for game-specific shaders, entities etc. It implement PVS and frustum culling so performance is optimal. At the moment only Quake III Arena maps are supported but an interface is provided for adding other BSP versions in the future.

![Screenshot](http://kondrak.info/images/qbsp/qbsp1.png?raw=true)

Usage
-----
Running the viewer:

<code>QuakeBspViewer.exe <path-to-bsp-file> </code>

Use tilde key (~) to toggle statistics menu on/off. Note that you must have Quake III Arena textures and models unpacked in the root directory if you want to see proper texturing. To move around use the WASD keys. RF keys lift you up/down and QE keys let you do the barrel roll.

Dependencies
-------
This project uses following external libraries:

- [stb_image](https://github.com/nothings/stb) library for image handling (c) Sean Barret
- SDL2 library for window/input
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for painless memory management of Vulkan resources

TODO
-------
- take advantage of SDLs recently added Vulkan functions
- add MSAA
- use pipeline derivatives
- add more comments?