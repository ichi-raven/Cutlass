#pragma once

#include <optional>
#include <variant>
#include <vector>
#include "Utility.hpp"
#include "RenderPipeline.hpp"

namespace Cutlass
{
    using ClearValue = std::vector<std::vector<float>>;

    enum class CommandType
    {
        eCmdBeginRenderPipeline,
        eCmdEndRenderPipeline,
        eCmdSetVB,
        eCmdSetIB,
        eCmdSetShaderResource,
        eCmdRender,
    };

    struct CmdBeginRenderPipeline
    {
        HRenderPipeline RPHandle;
    };

    struct CmdEndRenderPipeline
    {
        uint32_t sync;//なんとなく作ってみた
        //指定するものがあれば
    };

    struct CmdSetVB
    {
        HBuffer VBHandle;
    };

    struct CmdSetIB
    {
        HBuffer IBHandle;
    };

    struct CmdSetShaderResource
    {
        ShaderResource shaderResource;
    };

    struct CmdRender
    {
        uint32_t firstIndex;//何番目のインデックスから描画を開始するか
        uint32_t indexCount;//いくつインデックスを描画するか
        uint32_t instanceCount;//インスタンシング描画しない場合は1
        uint32_t vertexOffset;//描画し終わった頂点だけずらす、普通は0
        uint32_t firstInstance;//インスタシング描画しないなら0
    };

    struct Command
    {
        CommandType type;//型に対応するenumを代入してください
        std::variant
        <
            CmdBeginRenderPipeline, 
            CmdEndRenderPipeline,
            CmdSetVB, 
            CmdSetIB, 
            CmdSetShaderResource, 
            CmdRender
        > info;//実際の構造体を代入してください
    };
}            