#pragma once

#include "Utility.hpp"

#include "RenderPass.hpp"
#include "Shader.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <queue>
#include <string>
#include <vector>

namespace Cutlass
{
    using Viewport = std::array<std::array<float, 3>, 2>;
    using Scissor  = std::array<std::array<float, 2>, 2>;

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
        eTriangleFan,
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
            : polygonMode(PolygonMode::eFill), cullMode(CullMode::eNone), frontFace(FrontFace::eClockwise), lineWidth(1.f)
        {
        }

        RasterizerState(
            PolygonMode polygonMode,
            CullMode cullMode,
            FrontFace frontFace,
            float lineWidth = 1.f)
            : polygonMode(polygonMode), cullMode(cullMode), frontFace(frontFace), lineWidth(lineWidth)
        {
        }

        bool operator==(const RasterizerState& other) const
        {
            return polygonMode == other.polygonMode &&
                   cullMode == other.cullMode &&
                   frontFace == other.frontFace &&
                   lineWidth == other.lineWidth;
        }

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

    struct GraphicsPipelineInfo
    {
        // GraphicsPipelineInfo
        //  (
        //      const Shader& VS,
        //      const Shader& FS,
        //      const HRenderPass& renderPass,
        //      const DepthStencilState& depthStencilState = DepthStencilState::eDepth,
        //      const RasterizerState& rasterizerState = RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
        //      const Topology& topology = Topology::eTriangleList,
        //      const ColorBlend& colorBlend = ColorBlend::eDefault,
        //      const MultiSampleState& multiSampleState = MultiSampleState::eDefault
        //  )
        //      : colorBlend(colorBlend)
        //      , topology(topology)
        //      , rasterizerState(rasterizerState)
        //      , multiSampleState(multiSampleState)
        //      , depthStencilState(depthStencilState)
        //      , VS(VS)
        //      , FS(FS)
        //      , renderPass(renderPass)
        //  {

        // }

        GraphicsPipelineInfo()
        {
        }

        GraphicsPipelineInfo(
            const Shader& VS,
            const Shader& FS,
            const HRenderPass& renderPass,
            const DepthStencilState& depthStencilState = DepthStencilState::eDepth,
            const RasterizerState& rasterizerState     = RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
            const Topology& topology                   = Topology::eTriangleList,
            const ColorBlend& colorBlend               = ColorBlend::eDefault,
            const MultiSampleState& multiSampleState   = MultiSampleState::eDefault)
            : colorBlend(colorBlend), topology(topology), rasterizerState(rasterizerState), multiSampleState(multiSampleState), depthStencilState(depthStencilState), VS(VS), FS(FS), renderPass(renderPass)
        {
        }

        bool operator==(const GraphicsPipelineInfo& other) const
        {
            return colorBlend == other.colorBlend &&
                   topology == other.topology &&
                   rasterizerState == other.rasterizerState &&
                   multiSampleState == other.multiSampleState &&
                   depthStencilState == other.depthStencilState &&
                   VS.getPath().compare(other.VS.getPath()) == 0 &&
                   FS.getPath().compare(other.FS.getPath()) == 0 &&
                   (viewport == other.viewport) && (viewport ? viewport.value() == other.viewport.value() : 1) &&
                   (scissor == other.scissor) && (scissor ? scissor.value() == other.scissor.value() : 1) &&
                   renderPass == other.renderPass;
        }

        ColorBlend colorBlend;
        Topology topology;
        RasterizerState rasterizerState;
        MultiSampleState multiSampleState;
        DepthStencilState depthStencilState;
        Shader VS;
        Shader FS;
        std::optional<Viewport> viewport;  //左上手前、右下奥3次元(Depthは正規化座標)
        std::optional<Scissor> scissor;    //左上、右下2次元
        HRenderPass renderPass;            //描画対象
        // RenderPass renderPass;
    };
};  // namespace Cutlass

// hash定義
template <typename T>
void combineHash(size_t& seed, T v)
{
    // from boost
    std::hash<T> primitive_type_hash;
    seed ^= primitive_type_hash(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
    template <>
    struct hash<Cutlass::GraphicsPipelineInfo>
    {
        size_t operator()(const Cutlass::GraphicsPipelineInfo& data) const
        {
            size_t seed = 0;
            combineHash(seed, static_cast<uint32_t>(data.colorBlend));
            combineHash(seed, static_cast<uint32_t>(data.topology));
            combineHash(seed, static_cast<uint32_t>(data.rasterizerState.cullMode));
            combineHash(seed, static_cast<uint32_t>(data.rasterizerState.frontFace));
            combineHash(seed, data.rasterizerState.lineWidth);
            combineHash(seed, static_cast<uint32_t>(data.rasterizerState.polygonMode));
            combineHash(seed, static_cast<uint32_t>(data.multiSampleState));
            combineHash(seed, static_cast<uint32_t>(data.depthStencilState));
            combineHash(seed, data.VS.getPath());
            combineHash(seed, data.FS.getPath());
            if (data.viewport)
                for (size_t i = 0; i < 3; ++i)
                    for (size_t j = 0; j < 2; ++j)
                        combineHash(seed, data.viewport.value()[i][j]);
            if (data.scissor)
                for (size_t i = 0; i < 2; ++i)
                    for (size_t j = 0; j < 2; ++j)
                        combineHash(seed, data.scissor.value()[i][j]);
            combineHash(seed, data.renderPass.getID());

            return seed;
        }
    };
}  // namespace std