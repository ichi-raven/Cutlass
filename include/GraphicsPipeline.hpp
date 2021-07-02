#pragma once

#include "Utility.hpp"

#include "Shader.hpp"
#include "RenderPass.hpp"

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

    struct  GraphicsPipelineInfo
    {
       //GraphicsPipelineInfo
        // (
        //     const Shader& VS,
        //     const Shader& FS,
        //     const HRenderPass& renderPass,
        //     const DepthStencilState& depthStencilState = DepthStencilState::eDepth,
        //     const RasterizerState& rasterizerState = RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
        //     const Topology& topology = Topology::eTriangleList,
        //     const ColorBlend& colorBlend = ColorBlend::eDefault,
        //     const MultiSampleState& multiSampleState = MultiSampleState::eDefault
        // )
        //     : colorBlend(colorBlend)
        //     , topology(topology)
        //     , rasterizerState(rasterizerState)
        //     , multiSampleState(multiSampleState)
        //     , depthStencilState(depthStencilState)
        //     , VS(VS)
        //     , FS(FS)
        //     , renderPass(renderPass)
        // {

        // }

        GraphicsPipelineInfo
        (
            const Shader& VS,
            const Shader& FS,
            const RenderPass& renderPass,
            const DepthStencilState& depthStencilState = DepthStencilState::eDepth,
            const RasterizerState& rasterizerState = RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
            const Topology& topology = Topology::eTriangleList,
            const ColorBlend& colorBlend = ColorBlend::eDefault,
            const MultiSampleState& multiSampleState = MultiSampleState::eDefault
        )
            : colorBlend(colorBlend)
            , topology(topology)
            , rasterizerState(rasterizerState)
            , multiSampleState(multiSampleState)
            , depthStencilState(depthStencilState)
            , VS(VS)
            , FS(FS)
            , renderPass(renderPass)
        {

        }

        ColorBlend colorBlend;
        Topology topology;
        RasterizerState rasterizerState;
        MultiSampleState multiSampleState;
        DepthStencilState depthStencilState;
        Shader VS;
        Shader FS;
        std::optional<Viewport> viewport; //左上手前、右下奥3次元(Depthは正規化座標)
        std::optional<Scissor> scissor;  //左上、右下2次元
		//HRenderPass renderPass;//描画対象
        RenderPass renderPass;
    };
    
};