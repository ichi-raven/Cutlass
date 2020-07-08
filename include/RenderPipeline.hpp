#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "Utility.hpp"

namespace Cutlass
{
    enum class RenderDSTType
    {
        eColor,
        eDepth,
    };

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
        eDefault,
    };

    struct VertexLayout
    {
        size_t sizeOfType;//型のサイズ
        std::vector<std::pair<ResourceType, const char*>> layouts;
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
        const char* path;
        const char* entryPoint;
    };

    struct RenderPipelineInfo
    {
        VertexLayout vertexLayout;
        ColorBlend colorBlend;
        Topology topology;
        RasterizerState rasterizerState;
        MultiSampleState multiSampleState;
        std::vector<std::pair<RenderDSTType, HTexture>> renderDSTs;
        Shader vertexShader;
        Shader fragmentShader;
        ShaderResourceLayout SRLayouts;
    };

};