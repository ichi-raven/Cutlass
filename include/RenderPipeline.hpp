#pragma once

#include "Utility.hpp"

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <optional>


namespace Cutlass
{
    // enum class RenderDSTType
    // {
    //     eColor,
    //     eDepth,
    // };

    using Viewport = std::array<std::array<float, 3>, 2>;
    using Scissor = std::array<std::array<float, 2>, 2>;

    enum class ColorBlend
    {
        eDefault,
    };

    enum class Topology
    {
        eTriangleList,
        eTriangleStrip,
    };

    enum class RasterizerState
    {
        eDefault,
    };

    enum class MultiSampleState
    {
        eDefault,
    };

    enum class DepthStencilState
    {
        eNone,
        eDepth,
        eStencil,
        eBoth,
    };

    struct VertexLayout
    {
        
        size_t sizeOfType; //全体としての型のサイズ
        std::vector<std::pair<ResourceType, std::string>> layouts;

        void add(const ResourceType &type, const std::string &name)
        {
            layouts.emplace_back(std::pair(type, name));
        }
    };

    struct ShaderResourceSetLayout
    {
        uint32_t uniformBufferCount;
        uint32_t combinedTextureCount;
    };

    struct ShaderResourceDesc
    {
        ShaderResourceSetLayout layout;
        uint32_t maxSetCount;
    };

    struct ShaderResourceSet
    {
        std::vector<HBuffer> uniformBuffer;
        std::vector<HTexture> combinedTexture;
    };

    struct Shader
    {
        std::string path;
        std::string entryPoint;
    };

    struct RenderPipelineInfo
    {
        std::optional<VertexLayout> vertexLayout;
        ColorBlend colorBlend;
        Topology topology;
        RasterizerState rasterizerState;
        MultiSampleState multiSampleState;
        DepthStencilState depthStencilState;
        Shader VS;
        Shader FS;
        ShaderResourceDesc SRDesc;
        std::optional<Viewport> viewport; //左上手前、右下奥3次元(Depthは正規化座標)
        std::optional<Scissor> scissor;  //左上、右下2次元
		HRenderDST renderDST;//描画対象
    };
    
};