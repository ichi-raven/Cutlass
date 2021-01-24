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
        eNone,
        eDefault,
        eAlphaBlend,
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
            , cullMode(CullMode::eNone)
            , frontFace(FrontFace::eClockwise)
            , lineWidth(1.f)
        {}

        RasterizerState
        (
            PolygonMode polygonMode,
            CullMode cullMode,
            FrontFace frontFace,
            float lineWidth = 1.f
        )
            : polygonMode(polygonMode)
            , cullMode(cullMode)
            , frontFace(frontFace)
            , lineWidth(lineWidth)
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

    using VertexElement = std::pair<ResourceType, std::string_view>;

    struct VertexLayout
    {
        VertexLayout(){}

        VertexLayout(std::initializer_list<VertexElement> elements)
        : layouts(elements)
        {}

        std::vector<VertexElement> layouts;

        void set(const ResourceType &type, const std::string_view &name)
        {
            layouts.emplace_back(std::pair(type, name));
        }
    };


    struct  GraphicsPipelineInfo
    {
        GraphicsPipelineInfo() {}

        GraphicsPipelineInfo
        (
            const ColorBlend& colorBlend,
            const Topology& topology,
            const RasterizerState& rasterizerState,
            const MultiSampleState& multiSampleState,
            const DepthStencilState& depthStencilState,
            const Shader& VS, const Shader& FS,
            const ShaderResourceDesc& SRDesc,
            const HRenderPass& renderPass
        )
            : colorBlend(colorBlend)
            , topology(topology)
            , rasterizerState(rasterizerState)
            , multiSampleState(multiSampleState)
            , depthStencilState(depthStencilState)
            , VS(VS)
            , FS(FS)
            , SRDesc(SRDesc)
            , renderPass(renderPass)
        {

        }

         GraphicsPipelineInfo
        (
            const VertexLayout& vertecLayout,
            const ColorBlend& colorBlend,
            const Topology& topology,
            const RasterizerState& rasterizerState,
            const MultiSampleState& multiSampleState,
            const DepthStencilState& depthStencilState,
            const Shader& VS, const Shader& FS,
            const ShaderResourceDesc& SRDesc,
            const HRenderPass& renderPass
        )
            : vertexLayout(vertecLayout)
            , colorBlend(colorBlend)
            , topology(topology)
            , rasterizerState(rasterizerState)
            , multiSampleState(multiSampleState)
            , depthStencilState(depthStencilState)
            , VS(VS)
            , FS(FS)
            , SRDesc(SRDesc)
            , renderPass(renderPass)
        {

        }

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
		HRenderPass renderPass;//描画対象
    };
    
};