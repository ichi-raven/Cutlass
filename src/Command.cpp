#include "Command.hpp"

namespace Cutlass
{

    void CommandList::beginRenderPipeline(const HRenderPipeline &RPHandle)
    {
        mCommands.emplace_back(CommandType::eBeginRenderPipeline, CmdBeginRenderPipeline{RPHandle});
    }

    void CommandList::endRenderPipeline()
    {
        mCommands.emplace_back(CommandType::eEndRenderPipeline, CmdEndRenderPipeline{});
    }

    void CommandList::bindVB(const HBuffer& VBHandle)
    {
        mCommands.emplace_back(CommandType::eBindVB, CmdBindVB{VBHandle});
    }

    void CommandList::bindIB(const HBuffer& IBHandle)
    {
        mCommands.emplace_back(CommandType::eBindIB, CmdBindIB{IBHandle});
    }

    void CommandList::bindSRSet(const ShaderResourceSet &SRSet)
    {
        ;
        mCommands.emplace_back(CommandType::eBindSRSet, CmdBindSRSet{SRSet});
    }

    void CommandList::renderIndexed
    ( 
            uint32_t firstIndex,
            uint32_t indexCount,    
            uint32_t instanceCount, 
            uint32_t vertexOffset,  
            uint32_t firstInstance 
    )
    {
       mCommands.emplace_back(CommandType::eRenderIndexed, CmdRenderIndexed{firstIndex, indexCount, instanceCount, vertexOffset, firstInstance});
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