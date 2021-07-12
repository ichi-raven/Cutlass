#include "../include/Command.hpp"

#include <iostream>

namespace Cutlass
{   
    void CommandList::begin(const HRenderPass& handle, bool clearFlag, const ColorClearValue ccv, const DepthClearValue dcv)
    {
        mCommands.emplace_back(CommandType::eBegin, CmdBegin{handle, ccv, dcv, clearFlag});
        begun = true;
    }

    void CommandList::begin(const HRenderPass& handle, const DepthClearValue dcv, const ColorClearValue ccv)
    {
        mCommands.emplace_back(CommandType::eBegin, CmdBegin{handle, ccv, dcv, true});
        begun = true;
    }

    void CommandList::end(bool presentIfRenderedFrameBuffer)
    {
        mCommands.emplace_back(CommandType::eEnd, CmdEnd{});
        if(presentIfRenderedFrameBuffer)
             mCommands.emplace_back(CommandType::ePresent, CmdPresent{});
        begun = false;
        indexed = false;
        graphicsPipeline = false;
    }

    void CommandList::bind(const HGraphicsPipeline& handle)
    {
        if(!begun)
        {
            std::cerr << "This command list is not begun!\n";
            return;
        }
        mCommands.emplace_back(CommandType::eBindGraphicsPipeline, CmdBindGraphicsPipeline{handle});
        graphicsPipeline = true;
    }

    void CommandList::bind(const HBuffer& VBHandle)
    {
        mCommands.emplace_back(CommandType::eBindVB, CmdBindVB{VBHandle});
    }

    void CommandList::bind(const HBuffer& VBHandle, const HBuffer& IBHandle)
    {
        mCommands.emplace_back(CommandType::eBindVB, CmdBindVB{VBHandle});
        mCommands.emplace_back(CommandType::eBindIB, CmdBindIB{IBHandle});

        indexed = true;
    }

    void CommandList::bind(const uint16_t set, const ShaderResourceSet& shaderResourceSet)
    {
        if(!graphicsPipeline)
        {
            std::cerr << "bind graphics pipeline first!\n";
            return;
        }

        mCommands.emplace_back(CommandType::eBindSRSet, CmdBindSRSet{set, shaderResourceSet});
    }

    void CommandList::bindIndexBuffer(const HBuffer& IBHandle)
    {
        mCommands.emplace_back(CommandType::eBindIB, CmdBindIB{IBHandle});
        indexed = true;
    }

    // void CommandList::present()
    // {
    //     mCommands.emplace_back(CommandType::ePresent, CmdPresent{});
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
        if(!begun)
        {
            std::cerr << "This command list is not begun!\n";
            return;
        }
        if(!indexed)
        {
            std::cerr << "index buffer is not set!\n";
            return;
        }
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
        if(!begun)
        {
            std::cerr << "This command list is not begun!\n";
            return;
        }
        mCommands.emplace_back(CommandType::eRender, CmdRender{vertexCount, instanceCount, vertexOffset, firstInstance});
    }

    void CommandList::barrier(const HTexture& handle)
    {
        mCommands.emplace_back(CommandType::eBarrier, CmdBarrier{handle});//, std::nullopt});
    }

    void CommandList::append(CommandList& commandList)
    {
        auto& icl = commandList.getInternalCommandData();
        mCommands.reserve(icl.size());
        std::copy(icl.begin(), icl.end(), std::back_inserter(mCommands));
    }

    void CommandList::executeSubCommand(const HCommandBuffer& handle)
    {
        if(!begun)
        {
            std::cerr << "This command list is not begun!\n";
            return;
        }

        mCommands.emplace_back(CommandType::eExecuteSubCommand, CmdExecuteSubCommand{handle});
    }

    const std::vector<std::pair<CommandType, CommandInfoVariant>>& CommandList::getInternalCommandData() const
    {
        return mCommands;
    }

    const bool CommandList::useSubCommand() const
    {
        return useSub;
    }

    //------------------------------------------------------------------

    SubCommandList::SubCommandList(const HRenderPass& usedInMainCommand)
    : mainRenderPass(usedInMainCommand)
    {

    }

    void SubCommandList::bind(const HGraphicsPipeline& handle)
    {
        mCommands.emplace_back(CommandType::eBindGraphicsPipeline, CmdBindGraphicsPipeline{handle});
        graphicsPipeline = true;
    }

    void SubCommandList::bind(const HBuffer& VBHandle)
    {
        mCommands.emplace_back(CommandType::eBindVB, CmdBindVB{VBHandle});
    }

    void SubCommandList::bind(const HBuffer& VBHandle, const HBuffer& IBHandle)
    {
        mCommands.emplace_back(CommandType::eBindVB, CmdBindVB{VBHandle});
        mCommands.emplace_back(CommandType::eBindIB, CmdBindIB{IBHandle});

        indexed = true;
    }

    void SubCommandList::bind(const uint16_t set, const ShaderResourceSet& shaderResourceSet)
    {
        if(!graphicsPipeline)
        {
            std::cerr << "bind graphics pipeline first!\n";
            return;
        }

        mCommands.emplace_back(CommandType::eBindSRSet, CmdBindSRSet{set, shaderResourceSet});
    }

    void SubCommandList::bindIndexBuffer(const HBuffer& IBHandle)
    {
        mCommands.emplace_back(CommandType::eBindIB, CmdBindIB{IBHandle});
        indexed = true;
    }

    void SubCommandList::renderIndexed
    ( 
            uint32_t indexCount,    
            uint32_t instanceCount, 
            uint32_t firstIndex,
            uint32_t vertexOffset,  
            uint32_t firstInstance 
    )
    {
        if(!indexed)
        {
            std::cerr << "index buffer is not set!\n";
            return;
        }
        mCommands.emplace_back(CommandType::eRenderIndexed, CmdRenderIndexed{indexCount, instanceCount, firstIndex, vertexOffset, firstInstance});
    }

    void SubCommandList::render
    (
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t vertexOffset,
        uint32_t firstInstance
    )
    {
        mCommands.emplace_back(CommandType::eRender, CmdRender{vertexCount, instanceCount, vertexOffset, firstInstance});
    }

    void SubCommandList::barrier(const HTexture& handle)
    {
        mCommands.emplace_back(CommandType::eBarrier, CmdBarrier{handle});//, std::nullopt});
    }

    void SubCommandList::append(SubCommandList& commandList)
    {
        auto& icl = commandList.getInternalCommandData();
        mCommands.reserve(icl.size());
        std::copy(icl.begin(), icl.end(), std::back_inserter(mCommands));
    }

    const std::vector<std::pair<CommandType, CommandInfoVariant>>& SubCommandList::getInternalCommandData() const
    {
        return mCommands;
    }

    const HRenderPass& SubCommandList::getRenderPass() const
    {
        return mainRenderPass;
    }
}