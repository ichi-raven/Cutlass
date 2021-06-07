#pragma once

#include "Utility.hpp"

#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <optional>

namespace Cutlass
{
    // struct ShaderResourceSetLayout
    // {
    //     void allocForUniformBuffer(uint8_t binding);
    //     void allocForCombinedTexture(uint8_t binding);

    //     const std::vector<uint8_t>& getUniformBufferBindings() const;
    //     const std::vector<uint8_t>& getCombinedTextureBindings() const;

    // private:
    //     std::vector<uint8_t> uniformBuffer;
    //     std::vector<uint8_t> combinedTexture;
    // };

    // struct ShaderResourceDesc
    // {
    //     ShaderResourceSetLayout layout;
    //     uint32_t setCount;
    // };

    struct ShaderResourceSet
    {
        void setUniformBuffer(uint8_t binding, const HBuffer& handle);
        void setCombinedTexture(uint8_t binding, const HBuffer &handle);
        
        const std::vector<std::pair<uint8_t, HBuffer>>& getUniformBuffers() const;
        const std::vector<std::pair<uint8_t, HTexture>>& getCombinedTextures() const;

    private:
        std::vector<std::pair<uint8_t, HBuffer>> uniformBuffers;
        std::vector<std::pair<uint8_t, HTexture>> combinedTextures;
    };

    struct Shader
    {
        Shader() {}

        Shader(const std::string_view path, const std::string_view entryPoint)
        {
            load(path, entryPoint);
        }

        void load(const std::string_view path, const std::string_view entryPoint);

        enum class ShaderResourceType
        {
            eUniformBuffer,
            eCombinedTexture,
            eSampler,
        };

        const std::vector<char>& getShaderByteCode() const;
        
        const std::string_view getEntryPoint() const;

        //key = <set, binding>, param = resource type
        const std::map<std::pair<uint8_t, uint8_t>, ShaderResourceType>& getLayoutTable() const;
        //first = type, second = semantic
        const std::vector<std::pair<ResourceType, std::optional<std::string>>>& getInputVariables() const;
        //first = type, second = semantic
        const std::vector<std::pair<ResourceType, std::optional<std::string>>>& getOutputVariables() const;

    private:
        std::vector<char> mFileData;
        std::string_view mEntryPoint;

        //<<set, binding>, resource type>
        std::map<std::pair<uint8_t, uint8_t>, ShaderResourceType> mResourceLayoutTable;
        //<type, semantic>
        std::vector<std::pair<ResourceType, std::optional<std::string>>> mInputVariables;
        std::vector<std::pair<ResourceType, std::optional<std::string>>> mOutputVariables;
    };
};