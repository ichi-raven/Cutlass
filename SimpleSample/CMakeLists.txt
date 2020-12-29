cmake_minimum_required(VERSION 3.11)

project(SimpleSample CXX)

add_definitions("-w -std=c++17")

include_directories(
   vulkan
   GLFW
   glm
   stb
   "../include"
)

link_directories(../build)

add_executable(App main.cpp)

target_link_libraries(App
   vulkan
   glfw
   cutlass
)
