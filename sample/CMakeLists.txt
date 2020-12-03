cmake_minimum_required(VERSION 3.11)

project(CutlassTest CXX)

add_definitions("-w -std=c++17")

include_directories(
   vulkan
   GLFW
   glm
   stb
   "../include"
)

add_executable(App main.cpp)

link_directories(../)

target_link_libraries(App
   vulkan
   glfw
   cutlass
)
