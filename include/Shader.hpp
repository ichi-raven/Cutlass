#pragma once

#include <cassert>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Utility.hpp"

namespace Cutlass
{
    //struct ShaderResourceSetLayout
    //{
    //void allocForUniformBuffer(uint8_t binding);
    //void allocForCombinedTexture(uint8_t binding);

    //const std::vector<uint8_t>& getUniformBufferBindings() const;
    //const std::vector<uint8_t>& getCombinedTextureBindings() const;

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
        void bind(uint8_t binding, const HBuffer& handle);
        void bind(uint8_t binding, const HTexture& handle);

        const std::map<uint8_t, HBuffer>& getUniformBuffers() const;
        const std::map<uint8_t, HTexture>& getCombinedTextures() const;

    private:
        std::map<uint8_t, HBuffer> uniformBuffers;
        std::map<uint8_t, HTexture> combinedTextures;
    };

    struct Shader
    {
        Shader() {}

        Shader(std::string_view path) { load(path); }

        Shader(std::string_view path, std::string_view entryPoint) { load(path, entryPoint); }

        void load(std::string_view path, std::string_view entryPoint);
        void load(std::string_view path);

        // void load(const char* path, const std::string_view entryPoint);

        enum class ShaderResourceType
        {
            eUniformBuffer,
            eCombinedTexture,
            eSampler,
        };

        const std::vector<char>& getShaderByteCode() const;

        std::string_view getEntryPoint() const;

        std::string_view getPath() const;

        // key = <set, binding>, param = resource type
        const std::map<std::pair<uint8_t, uint8_t>, ShaderResourceType>& getLayoutTable() const;
        // first = type, second = semantic
        const std::vector<std::pair<ResourceType, std::optional<std::string>>>& getInputVariables() const;
        // first = type, second = semantic
        const std::vector<std::pair<ResourceType, std::optional<std::string>>>& getOutputVariables() const;

    private:
        std::vector<char> mFileData;
        std::string mEntryPoint;
        std::string mPath;

        // <<set, binding>, resource type>
        std::map<std::pair<uint8_t, uint8_t>, ShaderResourceType> mResourceLayoutTable;
        // <type, semantic>
        std::vector<std::pair<ResourceType, std::optional<std::string>>> mInputVariables;
        std::vector<std::pair<ResourceType, std::optional<std::string>>> mOutputVariables;
    };
}  // namespace Cutlass