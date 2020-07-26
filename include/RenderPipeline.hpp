#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "Utility.hpp"

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
        size_t sizeOfType;//全体としての型のサイズ
        std::vector<std::pair<ResourceType, std::string>> layouts;
        //std::vector<std::string> names;
    };

    struct ShaderResourceLayout
    {
        uint32_t uniformBufferCount;
        uint32_t combinedTextureCount;
    };

    struct ShaderResource
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
        VertexLayout vertexLayout;
        ColorBlend colorBlend;
        Topology topology;
        RasterizerState rasterizerState;
        MultiSampleState multiSampleState;
        DepthStencilState depthStencilState;
        Shader vertexShader;
        Shader fragmentShader;
        ShaderResourceLayout SRLayouts;
        std::optional<Viewport> viewport; //左上手前、右下奥3次元(Depthは正規化座標)
        std::optional<Scissor> scissor;  //左上、右下2次元
		HRenderDST renderDST;//描画対象
    };
    
};