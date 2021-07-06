
cmake_minimum_required(VERSION 3.11)

project(Cutlass C CXX)

add_definitions("-g -std=c++17")

include_directories(
   vulkan
   glfw
   ${PROJECT_SOURCE_DIR}/include
)

file(GLOB_RECURSE HDRS include/*.h*)
file(GLOB_RECURSE HDRS_THIRDPARTY include/ThirdParty/*.h*)
file(GLOB_RECURSE SRCS src/*.c*)

add_library(
   cutlass STATIC
   ${SRCS}
)

TARGET_LINK_LIBRARIES(
   cutlass
   vulkan
   glfw
)

install(TARGETS cutlass ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(FILES ${HDRS} DESTINATION include/Cutlass)
install(FILES ${HDRS_THIRDPARTY} DESTINATION include/Cutlass/ThirdParty)
