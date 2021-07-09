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

    //struct CmdBeginRenderPass
    // {
    //     HRenderPass RPHandle;
    //     ColorClearValue ccv;
    //     DepthClearValue dcv;
    //     bool clear;
    // };

    struct CmdBegin
    {
        HRenderPass handle;
        ColorClearValue ccv;
        DepthClearValue dcv;
        bool clear;
    };

    struct CmdBindGraphicsPipeline
    {
        HGraphicsPipeline handle;
    };

    struct CmdEnd
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

    // struct CmdBindUniformBuffer
    // {
    //     uint32_t set;
    //     uint32_t binding;
    //     HBuffer hBuffer;
    // };

    // struct CmdBindTexture
    // {
    //     uint32_t set;
    //     uint32_t binding;
    //     HTexture hTexture;
    // };

    struct CmdBindSRSet
    {
        uint16_t set;
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

    // struct ImDrawData;

    // struct CmdRenderImGui
    // {
    //     ImDrawData* data;
    // };

    struct CmdBarrier
    {
        HTexture handle;
    };

    //コマンド追加時はここ
    enum class CommandType
    {
        eBegin,
        eEnd,
        eBindGraphicsPipeline,
        ePresent,
        eBindVB,
        eBindIB,
        eBindSRSet,
        eRenderIndexed,
        eRender,
        //eRenderImGui,
        eBarrier,
    };

    //とここ
    using CommandInfoVariant = std::variant
    <
        CmdBegin,
        CmdEnd,
        CmdBindGraphicsPipeline,
        CmdPresent,
        CmdBindVB,
        CmdBindIB,
        CmdBindSRSet,
        CmdRenderIndexed,
        CmdRender,
        //CmdRenderImGui,
        CmdBarrier
    >;

    using InternalCommandList = std::vector<std::pair<CommandType, CommandInfoVariant>>;

    //ただのヘルパークラス 要速度検証
    class CommandList
    {
    public:
        CommandList()
        : indexed(false)
        , begun(false)
        , graphicsPipeline(false)
        {

        }
        // void beginRenderPass(const HRenderPass& RPHandle, bool clearFlag, const ColorClearValue ccv = { 0.2f, 0.2f, 0.2f, 1.f }, const DepthClearValue dcv = { 1.f, 0 });
        // void beginRenderPass(const HRenderPass& RPHandle, const DepthClearValue dcv = { 1.f, 0 }, const ColorClearValue ccv = { 0.2f, 0.2f, 0.2f, 1.f });

        void begin(const HRenderPass& handle, bool clearFlag, const ColorClearValue ccv = { 0.2f, 0.2f, 0.2f, 1.f }, const DepthClearValue dcv = { 1.f, 0 });
        void begin(const HRenderPass& handle, const DepthClearValue dcv = { 1.f, 0 }, const ColorClearValue ccv = { 0.2f, 0.2f, 0.2f, 1.f });
        void end(bool presentIfRenderedFrameBuffer = true);
        
        //void present();
        //void bindGraphicsPipeline(const HGraphicsPipeline& handle);
        // void bindVertexBuffer(const HBuffer& handle);
        // void bindIndexBuffer(const HBuffer& handle);

        // void bindShaderResourceSet(const uint32_t set, const ShaderResourceSet &shaderResourceSet);
        
        void bind(const HGraphicsPipeline& handle);

        //bind vertex buffer only
        void bind(const HBuffer& VBHandle);
        //bind vertex buffer and index buffer
        void bind(const HBuffer& VBHandle, const HBuffer& IBHandle);
        //bind shader resource set
        void bind(const uint16_t set, const ShaderResourceSet& shaderSet);

        //option
        void bindIndexBuffer(const HBuffer& IBHandle);

        void renderIndexed
        (
            uint32_t indexCount,    //いくつインデックスを描画するか
            uint32_t instanceCount, //インスタンシング描画しない場合は1
            uint32_t firstIndex,    //何番目のインデックスから描画を開始するか
            uint32_t vertexOffset,  //描画し終わった頂点だけずらす、普通は0
            uint32_t firstInstance  //インスタンシング描画しないなら0
        );
        void render
        (
            uint32_t vertexCount,   //描画する頂点の個数
            uint32_t instanceCount, //インスタンシング描画しない場合は1
            uint32_t vertexOffset,  //描画し終わった頂点だけずらす、普通は0
            uint32_t firstInstance //インスタンシング描画しないなら0
        );

        //void renderImGui(ImDrawData* data = nullptr);

        void barrier(const HTexture& handle);

        //現在のCommandListに接続する
        void append(CommandList& commandList);

        const InternalCommandList& getInternalCommandData() const;

    private:
        InternalCommandList mCommands;
        bool indexed;
        bool begun;
        bool graphicsPipeline;
    };
}            