#pragma once

#include "Utility.hpp"

#include "Shader.hpp"

#include <vector>
#include <queue>
#include <array>
#include <string>
#include <cstdint>
#include <optional>


namespace Cutlass
{
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

    enum class PolygonMode
    {
        eFill,
        eLine,
        ePoint
    };

    enum class CullMode
    {
        eNone,
        eBack,
        eFront
    };

    enum class FrontFace
    {
        eClockwise,
        eCounterClockwise,
    };

    struct RasterizerState
    {
        RasterizerState()
            : polygonMode(PolygonMode::eFill)
            , cullMode(CullMode::eBack)
            , frontFace(FrontFace::eClockwise)
            , lineWidth(1.f)
        {}

        PolygonMode polygonMode;
        CullMode cullMode;
        FrontFace frontFace;
        float lineWidth;
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

    using VertexElement = std::pair<ResourceType, std::string>;

    struct VertexLayout
    {
        std::vector<VertexElement> layouts;

        void set(const ResourceType &type, const std::string &name)
        {
            layouts.emplace_back(std::pair(type, name));
        }
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