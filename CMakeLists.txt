
cmake_minimum_required(VERSION 3.11)

project(Cutlass C CXX)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wendif-labels -Wfloat-equal -Winit-self -Winline -Wlogical-op -Wmissing-include-dirs -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-promo -Wswitch-default -Wswitch-enum -Wunsafe-loop-optimizations -Wvariadic-macros -Wwrite-strings ")
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++17 -g3 -O0 -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++17 -O2 -s -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++17 -g3 -Og -pg")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-std=c++17 -Os -s -DNDEBUG -march=native")

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
install(DIRECTORY include/ThirdParty DESTINATION include/Cutlass)
