#include "../include/Command.hpp"

namespace Cutlass
{

    void CommandList::beginRenderPipeline(const HRenderPipeline& RPHandle, bool clearFlag, const ColorClearValue ccv, const DepthClearValue dcv)
    {
        mCommands.emplace_back(CommandType::eBeginRenderPipeline, CmdBeginRenderPipeline{RPHandle, ccv, dcv, clearFlag});
    }

    void CommandList::beginRenderPipeline(const HRenderPipeline& RPHandle, const DepthClearValue dcv, const ColorClearValue ccv)
    {
        mCommands.emplace_back(CommandType::eBeginRenderPipeline, CmdBeginRenderPipeline{RPHandle, ccv, dcv, true});
    }

    void CommandList::endRenderPipeline()
    {
        mCommands.emplace_back(CommandType::eEndRenderPipeline, CmdEndRenderPipeline{});
    }

    void CommandList::present()
    {
        mCommands.emplace_back(CommandType::ePresent, CmdPresent{});
    }

    void CommandList::bindVB(const HBuffer& VBHandle)
    {
        mCommands.emplace_back(CommandType::eBindVB, CmdBindVB{VBHandle});
    }

    void CommandList::bindIB(const HBuffer& IBHandle)
    {
        mCommands.emplace_back(CommandType::eBindIB, CmdBindIB{IBHandle});
    }

    void CommandList::bindSRSet(const ShaderResourceSet& shaderResourceSet)
    {
        mCommands.emplace_back(CommandType::eBindSRSet, CmdBindSRSet{shaderResourceSet});
    }

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

    void CommandList::sync()
    {
        mCommands.emplace_back(CommandType::eSync, CmdSync{});
    }

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