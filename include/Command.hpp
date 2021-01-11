#pragma once

#include <optional>
#include <variant>
#include <queue>
#include <tuple>
#include "Utility.hpp"
#include "GraphicsPipeline.hpp"

namespace Cutlass
{
    using ColorClearValue = std::array<float, 4>;
    using DepthClearValue = std::tuple<float, uint32_t>;

    struct CmdBeginRenderPass
    {
        HRenderPass RPHandle;
        ColorClearValue ccv;
        DepthClearValue dcv;
        bool clear;
    };

    struct CmdBindGraphicsPipeline
    {
        HGraphicsPipeline RPHandle;
    };

    struct CmdEndRenderPass
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
        HTexture hTexture;
    };

    //コマンド追加時はここ
    enum class CommandType
    {
        eBeginRenderPass,
        eEndRenderPass,
        eBindGraphicsPipeline,
        ePresent,
        eBindVB,
        eBindIB,
        eBindSRSet,
        eRenderIndexed,
        eRender,
        eSync,
    };

    //とここ
    using CommandInfoVariant = std::variant
    <
        CmdBeginRenderPass,
        CmdEndRenderPass,
        CmdBindGraphicsPipeline,
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
        void beginRenderPass(const HRenderPass& RPHandle, bool clearFlag, const ColorClearValue ccv = { 0.2f, 0.2f, 0.2f, 1.f }, const DepthClearValue dcv = { 1.f, 0 });
        void beginRenderPass(const HRenderPass& RPHandle, const DepthClearValue dcv = { 1.f, 0 }, const ColorClearValue ccv = { 0.2f, 0.2f, 0.2f, 1.f });

        void endRenderPass();
        
        void present();
        void bindGraphicsPipeline(const HGraphicsPipeline& handle);
        void bindVertexBuffer(const HBuffer& handle);
        void bindIndexBuffer(const HBuffer &handle);
        void bindShaderResourceSet(const ShaderResourceSet &shaderResourceSet);
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

        void readBarrier(const HTexture& handle);

        //現在のCommandListに接続する
        void append(CommandList& commandList);

        const InternalCommandList& getInternalCommandData() const;

    private:
        InternalCommandList mCommands;
    };
}            