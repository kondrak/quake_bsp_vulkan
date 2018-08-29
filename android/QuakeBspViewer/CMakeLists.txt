# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Source file root directories
set(SRC_ROOT ../../src)
set(SDL2_DIR ../../contrib/SDL2-2.0.8)

include_directories(../../contrib)
include_directories(${SDL2_DIR}/include)
include_directories(${SRC_ROOT})

# Application sources
file(GLOB APP_SRC "${SRC_ROOT}/*.cpp"
                  "${SRC_ROOT}/q3bsp/*.cpp"
                  "${SRC_ROOT}/renderer/*.cpp"
                  "${SRC_ROOT}/renderer/vulkan/*.cpp"
                  "../../contrib/stb_image/stb_image.c"
                  )

# SDL2 shared library sources
file(GLOB SDL2_SRC "${SDL2_DIR}/src/*.c"
                   "${SDL2_DIR}/src/audio/*.c"
                   "${SDL2_DIR}/src/audio/android/*.c"
                   "${SDL2_DIR}/src/audio/dummy/*.c"
                   "${SDL2_DIR}/src/atomic/*.c"
                   "${SDL2_DIR}/src/core/android/*.c"
                   "${SDL2_DIR}/src/cpuinfo/*.c"
                   "${SDL2_DIR}/src/dynapi/*.c"
                   "${SDL2_DIR}/src/events/*.c"
                   "${SDL2_DIR}/src/file/*.c"
                   "${SDL2_DIR}/src/haptic/*.c"
                   "${SDL2_DIR}/src/haptic/android/*.c"
                   "${SDL2_DIR}/src/joystick/*.c"
                   "${SDL2_DIR}/src/joystick/android/*.c"
                   "${SDL2_DIR}/src/joystick/steam/SDL_steamcontroller.c"
                   "${SDL2_DIR}/src/loadso/dlopen/*.c"
                   "${SDL2_DIR}/src/power/*.c"
                   "${SDL2_DIR}/src/power/android/*.c"
                   "${SDL2_DIR}/src/filesystem/android/*.c"
                   "${SDL2_DIR}/src/render/*.c"
                   "${SDL2_DIR}/src/render/*/*.c"
                   "${SDL2_DIR}/src/stdlib/*.c"
                   "${SDL2_DIR}/src/thread/*.c"
                   "${SDL2_DIR}/src/thread/pthread/*.c"
                   "${SDL2_DIR}/src/timer/*.c"
                   "${SDL2_DIR}/src/timer/unix/*.c"
                   "${SDL2_DIR}/src/video/*.c"
                   "${SDL2_DIR}/src/video/android/*.c"
                   "${SDL2_DIR}/src/video/yuv2rgb/*.c"
                   )

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library(main SHARED ${APP_SRC} )
add_library(SDL2 SHARED ${SDL2_SRC})

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       main
                       SDL2
                       vulkan )

target_link_libraries( # Specifies the target library.
                       SDL2
                       android
                       GLESv1_CM 
                       GLESv2
                       log )
