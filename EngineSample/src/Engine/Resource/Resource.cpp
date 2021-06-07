#include <Engine/Resources/Resources.hpp>

const std::string Engine::Resource::Shader::genPath(const char* target)
{
    return std::string(shaderDir) + std::string(target);
}

const std::string Engine::Resource::Model::genPath(const char* target)
{
    return std::string(modelDir) + std::string(target);
}