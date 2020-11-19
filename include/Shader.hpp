#pragma once

#include "Utility.hpp"

#include <vector>
#include <string>

namespace Cutlass
{
    struct ShaderResourceSetLayout
    {
        void allocForUniformBuffer(uint8_t binding);
        void allocForCombinedTexture(uint8_t binding);

        const std::vector<uint8_t>& getUniformBufferBindings() const;
        const std::vector<uint8_t>& getCombinedTextureBindings() const;

    private:
        std::vector<uint8_t> uniformBuffer;
        std::vector<uint8_t> combinedTexture;
    };

    struct ShaderResourceDesc
    {
        ShaderResourceSetLayout layout;
        uint32_t setCount;
    };

    struct ShaderResourceSet
    {
        void setUniformBuffer(uint8_t binding, HBuffer& handle);
        void setCombinedTexture(uint8_t binding, HBuffer &handle);

        const std::vector<std::pair<uint8_t, HBuffer>>& getUniformBuffers() const;
        const std::vector<std::pair<uint8_t, HTexture>>& getCombinedTextures() const;

    private:
        std::vector<std::pair<uint8_t, HBuffer>> uniformBuffers;
        std::vector<std::pair<uint8_t, HTexture>> combinedTextures;
    };

    struct Shader
    {
        Shader() {}

        Shader(const std::string& path, const std::string& entryPoint)
            : path(path)
            , entryPoint(entryPoint)

        {

        }

        Shader(const char* path, const char* entryPoint)
            : path(std::string(path))
            , entryPoint(std::string(entryPoint))
        {

        }

        std::string path;
        std::string entryPoint;
    };
};