#include "../include/Shader.hpp"

#include <iostream>

namespace Cutlass
{
    void ShaderResourceSetLayout::allocForUniformBuffer(uint8_t binding)
    {
        uniformBuffer.emplace_back(binding);
    }

    void ShaderResourceSetLayout::allocForCombinedTexture(uint8_t binding)
    {
        combinedTexture.emplace_back(binding);
    }

    const std::vector<uint8_t>& ShaderResourceSetLayout::getUniformBufferBindings() const
    {
        return uniformBuffer;
    }

    const std::vector<uint8_t>& ShaderResourceSetLayout::getCombinedTextureBindings() const
    {
        return combinedTexture;
    }


    void ShaderResourceSet::setUniformBuffer(uint8_t binding, const HBuffer& handle)
    {
        uniformBuffers.emplace_back(binding, handle);
    }

    void ShaderResourceSet::setCombinedTexture(uint8_t binding, const HBuffer& handle)
    {
        combinedTextures.emplace_back(binding, handle);
    }

    const std::vector<std::pair<uint8_t, HBuffer>>& ShaderResourceSet::getUniformBuffers() const
    {
        return uniformBuffers;
    }

    const std::vector<std::pair<uint8_t, HTexture>> &ShaderResourceSet::getCombinedTextures() const
    {
        return combinedTextures;
    }
};