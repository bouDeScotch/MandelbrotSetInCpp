cmake_minimum_required(VERSION 3.30)
project(untitled)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

add_executable(untitled WIN32 ./src/main.cpp)

target_include_directories(untitled PUBLIC "${CMAKE_SOURCE_DIR}/include/")

# Trouve SDL2 via vcpkg
find_package(SDL2 CONFIG REQUIRED)

# Lie SDL2 à ton exécutable
target_link_libraries(untitled PRIVATE SDL2::SDL2 SDL2::SDL2main)
