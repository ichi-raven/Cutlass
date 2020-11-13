#pragma once

#include <optional>
#include <variant>
#include <queue>
#include <tuple>
#include "Utility.hpp"
#include "RenderPipeline.hpp"

namespace Cutlass
{
    using ColorClearValue = std::array<float, 4>;
    using DepthClearValue = std::tuple<float, uint32_t>;

    struct CmdBeginRenderPipeline
    {
        HRenderPipeline RPHandle;
        ColorClearValue ccv;
        DepthClearValue dcv;
    };

    struct CmdEndRenderPipeline
    {

    };

    struct CmdPresent
    {
        
    };

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

    struct CmdSync
    {
        //HRender target;
    };

    //struct CmdSyncBuffer
    //{

    //};

    //コマンド追加時はここ
    enum class CommandType
    {
        eBeginRenderPipeline,
        eEndRenderPipeline,
        ePresent,
        eBindVB,
        eBindIB,
        eBindSRSet,
        eRenderIndexed,
        eRender,
        eSync,
        //eSyncBuffer,
    };

    //とここ
    using CommandInfoVariant = std::variant
    <
        CmdBeginRenderPipeline,
        CmdEndRenderPipeline,
        CmdPresent,
        CmdBindVB,
        CmdBindIB,
        CmdBindSRSet,
        CmdRenderIndexed,
        CmdRender,
        CmdSync
    >;

    using InternalCommandList = std::vector<std::pair<CommandType, CommandInfoVariant>>;

    //ただのヘルパークラス 要速度検証
    class CommandList
    {
    public:
        void beginRenderPipeline(const HRenderPipeline& RPHandle, const ColorClearValue ccv = { 0.2f, 0.2f, 0.2f, 1.f }, const DepthClearValue dcv = { 1.f, 0 });
        void endRenderPipeline();
        void present();
        void bindVB(const HBuffer &VBHandle);
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
        void sync();

        const InternalCommandList& getInternalCommandData() const;

    private:
        InternalCommandList mCommands;
    };
}            