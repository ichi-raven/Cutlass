#pragma once

#include <optional>
#include <variant>
#include <queue>
#include "Utility.hpp"
#include "RenderPipeline.hpp"

namespace Cutlass
{
    using ClearValue = std::vector<std::vector<float>>;

    struct CmdBindRenderPipeline
    {
        HRenderPipeline RPHandle;
    };

    //struct CmdEndRenderPipeline
    //{
    //    uint32_t sync;//なんとなく作ってみた
    //    //指定するものがあれば
    //};

    struct CmdBindVB
    {
        HBuffer VBHandle;
    };

    struct CmdBindIB
    {
        HBuffer IBHandle;
    };

    struct CmdBindSRSet
    {
        ShaderResourceSet SRSet;
    };

    struct CmdRenderIndexed
    {
        uint32_t indexCount;//いくつインデックスを描画するか
        uint32_t instanceCount;//インスタンシング描画しない場合は1
        uint32_t firstIndex;//何番目のインデックスから描画を開始するか
        uint32_t vertexOffset;//描画し終わった頂点だけずらす、普通は0
        uint32_t firstInstance;//インスタシング描画しないなら0
    };

    struct CmdRender
    {
        uint32_t vertexCount; //描画する頂点の個数
        uint32_t instanceCount; //インスタンシング描画しない場合は1
        uint32_t vertexOffset;  //描画し終わった頂点だけずらす、普通は0
        uint32_t firstInstance; //インスタシング描画しないなら0
    };

    //コマンド追加時はここ
    enum class CommandType
    {
        eBindRenderPipeline,
        //eEndRenderPipeline,
        eBindVB,
        eBindIB,
        eBindSRSet,
        eRenderIndexed,
        eRender,
    };

    //とここ
    using CommandInfoVariant = std::variant<
        CmdBindRenderPipeline,
        //CmdEndRenderPipeline,
        CmdBindVB,
        CmdBindIB,
        CmdBindSRSet,
        CmdRenderIndexed,
        CmdRender>;

    using InternalCommandList = std::vector<std::pair<CommandType, CommandInfoVariant>>;

    //ただのヘルパークラス 要速度検証
    class CommandList
    {
    public:
        void bindRenderPipeline(const HRenderPipeline& RPHandle);
        //void endRenderPipeline();
        void bindVB(const HBuffer& VBHandle);
        void bindIB(const HBuffer &IBHandle);
        void bindSRSet(const ShaderResourceSet &shaderResourceSet);
        void renderIndexed
        (
            uint32_t indexCount,    //いくつインデックスを描画するか
            uint32_t instanceCount, //インスタンシング描画しない場合は1
            uint32_t firstIndex,    //何番目のインデックスから描画を開始するか
            uint32_t vertexOffset,  //描画し終わった頂点だけずらす、普通は0
            uint32_t firstInstance  //インスタシング描画しないなら0
        );

        void render
        (
            uint32_t vertexCount,   //描画する頂点の個数
            uint32_t instanceCount, //インスタンシング描画しない場合は1
            uint32_t vertexOffset,  //描画し終わった頂点だけずらす、普通は0
            uint32_t firstInstance //インスタシング描画しないなら0
        );

        const InternalCommandList& getInternalCommandData() const;

    private:
        InternalCommandList mCommands;
    };


    // struct Command
    // {
    //     CommandType type;//型に対応するenumを代入してください
    //     std::variant
    //     <
    //         CmdBeginRenderPipeline, 
    //         CmdEndRenderPipeline,
    //         CmdSetVB, 
    //         CmdSetIB, 
    //         CmdSetShaderResource, 
    //         CmdRender
    //     > info;//実際の構造体を代入してください
    // };
}            