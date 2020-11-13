#pragma once

#include <cstdint>

namespace Cutlass
{
    using HWindow = uint32_t;
    using HBuffer = uint32_t;
    using HTexture = uint32_t;
    //using HSampler = uint32_t;
	using HRenderDST = uint32_t;
    using HRenderPipeline = uint32_t;
    using HCommandBuffer = uint32_t;

    enum class Result
    {
        eFailure = 0,
        eSuccess,
    };

    enum class ResourceType //とりあえずglmを想定していたりする
    {
        eUint32,
        eInt32,
        eUnorm,
        eFloat32,
        eF32Vec2,
        eF32Vec3,
        eF32Vec4,
        eU32Vec2,
        eU32Vec3,
        eU32Vec4,
        eS32Vec2,
        eS32Vec3,
        eS32Vec4,
        eUNormVec2,
        eUNormVec3,
        eUNormVec4,
        eFMat2,
        eFMat3,
        eFMat4,
    };
};