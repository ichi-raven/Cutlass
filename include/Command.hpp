#pragma once

#include <optional>
#include <queue>
#include <tuple>
#include <variant>

#include "GraphicsPipeline.hpp"
#include "Utility.hpp"

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
        uint32_t indexCount;     //いくつインデックスを描画するか
        uint32_t instanceCount;  //インスタンシング描画しない場合は1
        uint32_t firstIndex;     //何番目のインデックスから描画を開始するか
        uint32_t vertexOffset;   //描画し終わった頂点だけずらす、普通は0
        uint32_t firstInstance;  //インスタシング描画しないなら0
    };

    struct CmdRender
    {
        uint32_t vertexCount;    //描画する頂点の個数
        uint32_t instanceCount;  //インスタンシング描画しない場合は1
        uint32_t vertexOffset;   //描画し終わった頂点だけずらす、普通は0
        uint32_t firstInstance;  //インスタシング描画しないなら0
    };

    // struct ImDrawData;

    struct CmdRenderImGui
    {
    };

    struct CmdBarrier
    {
        HTexture handle;
    };

    struct CmdExecuteSubCommand
    {
        HCommandBuffer handle;
    };

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
        eRenderImGui,
        eBarrier,
        eExecuteSubCommand,
    };

    using CommandInfoVariant = std::variant<
        CmdBegin,
        CmdEnd,
        CmdBindGraphicsPipeline,
        CmdPresent,
        CmdBindVB,
        CmdBindIB,
        CmdBindSRSet,
        CmdRenderIndexed,
        CmdRender,
        CmdRenderImGui,
        CmdBarrier,
        CmdExecuteSubCommand>;

    using InternalCommandList = std::vector<std::pair<CommandType, CommandInfoVariant>>;

    class SubCommandList
    {
    public:
        SubCommandList(const HRenderPass& usedInMainCommand);

        void bind(const HGraphicsPipeline& handle);

        //bind vertex buffer only
        void bind(const HBuffer& VBHandle);
        //bind vertex buffer and index buffer
        void bind(const HBuffer& VBHandle, const HBuffer& IBHandle);
        //bind shader resource set
        void bind(const uint16_t set, const ShaderResourceSet& shaderSet);

        //option
        void bindIndexBuffer(const HBuffer& IBHandle);

        void renderIndexed(
            uint32_t indexCount,        
            uint32_t instanceCount = 1, 
            uint32_t firstIndex    = 0,
            uint32_t vertexOffset  = 0,
            uint32_t firstInstance = 0  
        );
        void render(
            uint32_t vertexCount,        
            uint32_t instanceCount = 1,
            uint32_t vertexOffset  = 0,
            uint32_t firstInstance = 0 
        );

        void renderImGui();

        void barrier(const HTexture& handle);

        void append(SubCommandList& commandList);

        const InternalCommandList& getInternalCommandData() const;

        void clear();

        const HRenderPass& getRenderPass() const;

        uint32_t getUniformBufferCount() const;

        uint32_t getCombinedTextureCount() const;

    private:
        InternalCommandList mCommands;
        bool indexed;
        bool graphicsPipeline;
        HRenderPass mainRenderPass;
        uint32_t uniformBufferCount;
        uint32_t combinedTextureCount;
    };

    class CommandList
    {
    public:
        CommandList()
            : indexed(false), begun(false), graphicsPipeline(false), useSub(false), uniformBufferCount(0), combinedTextureCount(0)
        {
        }

        void begin(const HRenderPass& handle, bool clearFlag, const ColorClearValue ccv = {0.2f, 0.2f, 0.2f, 0.f}, const DepthClearValue dcv = {1.f, 0});
        void begin(const HRenderPass& handle, const DepthClearValue dcv = {1.f, 0}, const ColorClearValue ccv = {0.2f, 0.2f, 0.2f, 1.f});
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

        void renderIndexed(
            uint32_t indexCount,         
            uint32_t instanceCount = 1,
            uint32_t firstIndex    = 0,
            uint32_t vertexOffset  = 0,
            uint32_t firstInstance = 0 
        );
        void render(
            uint32_t vertexCount,        
            uint32_t instanceCount = 1,
            uint32_t vertexOffset  = 0,
            uint32_t firstInstance = 0
        );

        void renderImGui();

        void barrier(const HTexture& handle);

        void executeSubCommand(const HCommandBuffer& handle);

        void append(CommandList& commandList);

        void clear();

        const InternalCommandList& getInternalCommandData() const;

        const bool useSubCommand() const;

        uint32_t getUniformBufferCount() const;

        uint32_t getCombinedTextureCount() const;

    private:
        InternalCommandList mCommands;
        bool indexed;
        bool begun;
        bool graphicsPipeline;
        bool useSub;
        uint32_t uniformBufferCount;
        uint32_t combinedTextureCount;
    };
}  // namespace Cutlass