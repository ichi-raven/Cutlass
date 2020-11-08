#include "../include/Command.hpp"

namespace Cutlass
{

    void CommandList::bindRenderPipeline(const HRenderPipeline &RPHandle)
    {
        mCommands.emplace_back(CommandType::eBindRenderPipeline, CmdBindRenderPipeline{RPHandle});
    }

    //void CommandList::endRenderPipeline()
    //{
    //    mCommands.emplace_back(CommandType::eEndRenderPipeline, CmdEndRenderPipeline{});
    //}

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
        mCommands.emplace_back(CommandType::eBindSRSet, CmdBindSRSet{SRSet});
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