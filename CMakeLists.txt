
cmake_minimum_required(VERSION 3.11)

add_definitions("-g -O0 -std=c++17")

INCLUDE_DIRECTORIES(
   "/usr/include/vulkan"
   "/usr/include/GLFW"
   "/usr/include/glm"
   "/usr/include/tinyobjloader"
   "/usr/include/stb"
   ${PROJECT_SOURCE_DIR}/include
)

FILE(GLOB SRCS src/*.cpp)
FILE(GLOB HDRS include/*.h)

add_executable(bin src/main.cpp)

TARGET_LINK_LIBRARIES(bin
   "/usr/lib/libvulkan.so"
   "/usr/lib/libglfw.so.3.2"
)
