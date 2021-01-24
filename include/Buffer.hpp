#pragma once

#include <cstdint>

namespace Cutlass
{
    enum class BufferUsage
    {
        eVertex,
        eIndex,
        eUniform,
    };

    struct BufferInfo
    {
        BufferInfo() {}

        BufferInfo(size_t bufferSize, BufferUsage usage, bool isHostVisible)
            : size(bufferSize)
            , usage(usage)
            , isHostVisible(isHostVisible)
        {

        }

        template<typename VertexType>
        inline void setVertexBuffer(size_t vertexCount)
        {
            size = vertexCount * sizeof(VertexType);
            usage = BufferUsage::eVertex;
            isHostVisible = true;
        }

        template<typename IndexType>
        inline void setIndexBuffer(size_t indexCount)
        {
            size = indexCount * sizeof(IndexType);
            usage = BufferUsage::eIndex;
            isHostVisible = true;
        }

        template<typename UniformType>
        inline void setUniformBuffer(size_t count = 1, bool _isHostVisible = true)
        {
            size = count * sizeof(UniformType);
            usage = BufferUsage::eUniform;
            isHostVisible = _isHostVisible;
        }

        BufferUsage usage;
        bool isHostVisible;
        size_t size;
    };

};