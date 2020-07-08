
cmake_minimum_required(VERSION 3.11)

project(CutlassEngine CXX)

add_definitions("-g -O0 -std=c++17")

INCLUDE_DIRECTORIES(
   vulkan
   glfw
   glm
   stb
   ${PROJECT_SOURCE_DIR}/include
)

FILE(GLOB SRCS src/*.cpp)
FILE(GLOB HDRS include/*.hpp)

add_library(cutlass STATIC ${SRCS})

TARGET_LINK_LIBRARIES(cutlass
   vulkan
   glfw
)

install(TARGETS cutlass ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(FILES include/Cutlass.hpp DESTINATION include)
