cmake_minimum_required(VERSION 3.20)
project(untitled)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

add_executable(MandelbrotSet
  src/main.cpp
  external/glad/src/glad.c
)

target_include_directories(MandelbrotSet PUBLIC "${CMAKE_SOURCE_DIR}/include/")

# Trouve SDL2 via vcpkg
find_package(SDL2 CONFIG REQUIRED)

# Lie SDL2 à ton exécutable
target_link_libraries(MandelbrotSet PRIVATE SDL2::SDL2 SDL2::SDL2main)

target_include_directories(MandelbrotSet PRIVATE external/glad/include)

target_compile_options(MandelbrotSet PRIVATE
    -O3
    -march=native
    -ffast-math
)
