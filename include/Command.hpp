#pragma once

#include <optional>
#include "Utility.hpp"
#include "RenderPipeline.hpp"

namespace Cutlass
{
    struct Command
    {
        std::optional<HBuffer> hVertexBuffer;
        std::optional<HBuffer> hIndexBuffer;
        HRenderPipeline hRenderPipeline;
        ShaderResource  shaderResource;
        uint32_t firstIndex;//何番目のインデックスから描画を開始するか
        uint32_t indexCount;//いくつインデックスを描画するか
        uint32_t instanceCount;//インスタンシング描画しない場合は1
        uint32_t vertexOffset;//描画し終わった頂点だけずらす、普通は0
        uint32_t firstInstance;//インスタシング描画しないなら0
    };
}