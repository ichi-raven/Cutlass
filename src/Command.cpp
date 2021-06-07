#include "../include/Command.hpp"

namespace Cutlass
{

    void CommandList::beginRenderPass(const HRenderPass& handle, bool clearFlag, const ColorClearValue ccv, const DepthClearValue dcv)
    {
        mCommands.emplace_back(CommandType::eBeginRenderPass, CmdBeginRenderPass{handle, ccv, dcv, clearFlag});
    }

    void CommandList::beginRenderPass(const HRenderPass& handle, const DepthClearValue dcv, const ColorClearValue ccv)
    {
        mCommands.emplace_back(CommandType::eBeginRenderPass, CmdBeginRenderPass{handle, ccv, dcv, true});
    }

    void CommandList::endRenderPass()
    {
        mCommands.emplace_back(CommandType::eEndRenderPass, CmdEndRenderPass{});
    }

    void CommandList::present()
    {
        mCommands.emplace_back(CommandType::ePresent, CmdPresent{});
    }

    void CommandList::bindGraphicsPipeline(const HGraphicsPipeline& handle)
    {
        mCommands.emplace_back(CommandType::eBindGraphicsPipeline, CmdBindGraphicsPipeline{handle});
    }

    void CommandList::bindVertexBuffer(const HBuffer& handle)
    {
        mCommands.emplace_back(CommandType::eBindVB, CmdBindVB{handle});
    }

    void CommandList::bindIndexBuffer(const HBuffer& handle)
    {
        mCommands.emplace_back(CommandType::eBindIB, CmdBindIB{handle});
    }

    void CommandList::bindShaderResourceSet(const uint32_t set, const ShaderResourceSet& shaderResourceSet)
    {
        mCommands.emplace_back(CommandType::eBindSRSet, CmdBindSRSet{set, shaderResourceSet});
    }
    // void CommandList::bindUniformBuffer(const uint32_t set, const uint32_t binding, const HBuffer& handle)
    // {
    //     mCommands.emplace_back(CommandType::eBindUB, CmdBindUniformBuffer{set, binding, handle});
    // }

    // void CommandList::bindTexture(const uint32_t set, const uint32_t binding, const HTexture& handle)
    // {
    //     mCommands.emplace_back(CommandType::eBindTex, CmdBindTexture{set, binding, handle});
    // }

    void CommandList::renderIndexed
    ( 
            uint32_t indexCount,    
            uint32_t instanceCount, 
            uint32_t firstIndex,
            uint32_t vertexOffset,  
            uint32_t firstInstance 
    )
    {
        mCommands.emplace_back(CommandType::eRenderIndexed, CmdRenderIndexed{indexCount, instanceCount, firstIndex, vertexOffset, firstInstance});
    }

    void CommandList::render
    (
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t vertexOffset,
        uint32_t firstInstance
    )
    {
        mCommands.emplace_back(CommandType::eRender, CmdRender{vertexCount, instanceCount, vertexOffset, firstInstance});
    }

    // void CommandList::readBarrierAll()
    // {
    //     mCommands.emplace_back(CommandType::eSync, CmdSync{true, std::nullopt});//, std::nullopt});
    // }

    void CommandList::readBarrier(const HTexture& handle)
    {
        mCommands.emplace_back(CommandType::eSync, CmdSync{handle});//, std::nullopt});
    }

    // void CommandList::readBarrier(const HBuffer& handle)
    // {
    //     mCommands.emplace_back(CommandType::eSync, CmdSync{true, std::nullopt, handle});
    // }

    void CommandList::append(CommandList& commandList)
    {
        auto& icl = commandList.getInternalCommandData();
        mCommands.reserve(icl.size());
        std::copy(icl.begin(), icl.end(), std::back_inserter(mCommands));
    }

    const std::vector<std::pair<CommandType, CommandInfoVariant>>& CommandList::getInternalCommandData() const
    {
        return mCommands;
    }
}