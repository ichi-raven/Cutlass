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
        BufferUsage usage;
        bool isHostVisible;
        size_t size;
    };
};