#pragma once

//Vulkan
#include<vulkan/vulkan.h>

//GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//GLM
// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtx/hash.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <array>
#include <optional>
#include <unordered_map>

#include <Device.hpp>
#include <Command.hpp>
#include <Buffer.hpp>
#include <Texture.hpp>
#include <Utility.hpp>
