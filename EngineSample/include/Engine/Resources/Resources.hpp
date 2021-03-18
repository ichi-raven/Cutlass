//Resources
#pragma once

#include <string>

namespace Engine
{
    constexpr const char* shaderDir = ("../resources/shaders/");
    const std::string genShaderPath(const char* shader)
    {
        static std::string path = std::string("../resources/shaders/");
        return path + std::string(shader);
    }
}