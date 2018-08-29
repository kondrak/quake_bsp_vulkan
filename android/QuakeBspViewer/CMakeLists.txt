# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(SRC ../../src)
set(SDL_DIR ../../contrib/SDL2-2.0.8)
set(SDL_SRC ${SDL_DIR}/src)
include_directories(../../contrib)
include_directories(${SDL_DIR}/include)
include_directories(${SRC})

file(GLOB SRC_LIST "${SRC}/*.cpp"
                   "${SRC}/q3bsp/*.cpp"
                   "${SRC}/renderer/*.cpp"
                   "${SRC}/renderer/vulkan/*.cpp"
                   "${SDL_SRC}/*.c"
                   "${SDL_SRC}/audio/*.c"
                   "${SDL_SRC}/audio/android/*.c"
                   "${SDL_SRC}/audio/dummy/*.c"
                   "${SDL_SRC}/atomic/*.c"
                   "${SDL_SRC}/core/android/*.c"
                   "${SDL_SRC}/cpuinfo/*.c"
                   "${SDL_SRC}/dynapi/*.c"
                   "${SDL_SRC}/events/*.c"
                   "${SDL_SRC}/file/*.c"
                   "${SDL_SRC}/haptic/*.c"
                   "${SDL_SRC}/haptic/android/*.c"
                   "${SDL_SRC}/joystick/*.c"
                   "${SDL_SRC}/joystick/android/*.c"
                   "${SDL_SRC}/joystick/steam/SDL_steamcontroller.c"
                   "${SDL_SRC}/loadso/dlopen/*.c"
                   "${SDL_SRC}/power/*.c"
                   "${SDL_SRC}/power/android/*.c"
                   "${SDL_SRC}/filesystem/android/*.c"
                   "${SDL_SRC}/render/*.c"
                   "${SDL_SRC}/render/*/*.c"
                   "${SDL_SRC}/stdlib/*.c"
                   "${SDL_SRC}/thread/*.c"
                   "${SDL_SRC}/thread/pthread/*.c"
                   "${SDL_SRC}/timer/*.c"
                   "${SDL_SRC}/timer/unix/*.c"
                   "${SDL_SRC}/video/*.c"
                   "${SDL_SRC}/video/android/*.c"
                   "${SDL_SRC}/video/yuv2rgb/*.c"
                   )

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library(main SHARED
            ${SRC_LIST}
            ../../contrib/stb_image/stb_image.c)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       main
                       #required by SDL2
                       android
                       GLESv1_CM 
                       GLESv2
                       #required by Vulkan
                       vulkan
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )