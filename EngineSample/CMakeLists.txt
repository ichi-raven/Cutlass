cmake_minimum_required(VERSION 3.11)

project(EngineSample CXX)

add_definitions("-w -std=c++17")

include_directories(
   vulkan
   GLFW
   glm
   stb
   cutlass
   "../include"
   "./include"
)

file(GLOB_RECURSE SRCS src/*.cpp)

link_directories(../build)

add_executable(App ${SRCS})


target_link_libraries(App
   vulkan
   glfw
   cutlass
)
