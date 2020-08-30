#pragma once

#include <cstdint>

namespace Cutlass
{
    using HSwapchain = uint32_t;
    using HBuffer = uint32_t;
    using HTexture = uint32_t;
    //using HSampler = uint32_t;
	using HRenderDST = uint32_t;
    using HRenderPipeline = uint32_t;

    enum class Result
    {
        eFailure = 0,
        eSuccess,
    };

    enum class ResourceType //とりあえずglmを想定していたりする
    {
        eUint32,
        eInt32,
        eFVec2,
        eFVec3,
        eFVec4,
        eMat2,
        eMat3,
        eMat4,
    };
};