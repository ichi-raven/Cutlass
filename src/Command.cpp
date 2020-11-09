#include "../include/Command.hpp"

namespace Cutlass
{

    void CommandList::beginRenderPipeline(const HRenderPipeline &RPHandle, const ColorClearValue& ccv, const DepthClearValue& dcv)
    {
        mCommands.emplace_back(CommandType::eBeginRenderPipeline, CmdBeginRenderPipeline{RPHandle, ccv, dcv});
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

    void CommandList::bindSRSet(const std::vector<ShaderResourceSet> &shaderResourceSets)
    {
        mCommands.emplace_back(CommandType::eBindSRSet, CmdBindSRSet{shaderResourceSets});
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

    const std::vector<std::pair<CommandType, CommandInfoVariant>>& CommandList::getInternalCommandData() const
    {
        return mCommands;
    }
}