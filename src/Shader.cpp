#include "../include/Shader.hpp"

#include <iostream>

namespace Cutlass
{
    // ShaderResourceSetLayout::ShaderResourceSetLayout()
    // {
    //     cursor = 0;
    // }

    void ShaderResourceSetLayout::addUniformBuffer(uint8_t binding)
    {
        uniformBuffer.emplace_back(binding);
        // if(cursor == 0)
        //     std::cerr << "warning : shader resource binding cursor overflowed\n";
    }

    void ShaderResourceSetLayout::addCombinedTexture(uint8_t binding)
    {
        combinedTexture.emplace_back(binding);
        // if (cursor == 0)
        //     std::cerr << "warning : shader resource binding cursor overflowed\n";
    }

    const std::vector<uint8_t>& ShaderResourceSetLayout::getUniformBufferBindings() const
    {
        return uniformBuffer;
    }

    const std::vector<uint8_t> &ShaderResourceSetLayout::getCombinedTextureBindings() const
    {
        return combinedTexture;
    }






    void ShaderResourceSet::setUniformBuffer(uint8_t binding, HBuffer &handle)
    {
        uniformBuffers.emplace_back(binding, handle);
    }

    void ShaderResourceSet::setCombinedTexture(uint8_t binding, HBuffer &handle)
    {
        combinedTextures.emplace_back(binding, handle);
    }

    const std::vector<std::pair<uint8_t, HBuffer>> &ShaderResourceSet::getUniformBuffers() const
    {
        return uniformBuffers;
    }

    const std::vector<std::pair<uint8_t, HTexture>> &ShaderResourceSet::getCombinedTextures() const
    {
        return combinedTextures;
    }
};