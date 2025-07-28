cmake_minimum_required(VERSION 3.30)
project(untitled)

set(CMAKE_CXX_STANDARD 20)

add_executable(untitled WIN32 ./src/main.cpp)

target_include_directories(untitled PUBLIC "${CMAKE_SOURCE_DIR}/include/")


find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(untitled ${SDL2_LIBRARIES})
