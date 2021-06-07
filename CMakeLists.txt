
cmake_minimum_required(VERSION 3.11)

project(Cutlass C CXX)

add_definitions("-g -std=c++17")

include_directories(
   vulkan
   glfw
   ${PROJECT_SOURCE_DIR}/include
)

file(GLOB HDRS include/*.hp*)

add_library(
   cutlass STATIC
   src/Context.cpp 
   src/Command.cpp 
   src/Event.cpp 
   src/Shader.cpp
   src/ThirdParty/spirv_reflect.c 
)

TARGET_LINK_LIBRARIES(
   cutlass
   vulkan
   glfw
)

install(TARGETS cutlass ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(FILES ${HDRS} DESTINATION include/Cutlass)
