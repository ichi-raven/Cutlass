
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
FILE(GLOB HDRS include/*.hpp)

add_library(cutlass STATIC ${SRCS})

TARGET_LINK_LIBRARIES(cutlass
   "/usr/lib/libvulkan.so"
   "/usr/lib/libglfw.so.3.2"
)

install(TARGETS cutlass ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
#install(FILES include/cutlass.hpp DESTINATION include)
