#pragma once

#include <cstdint>

namespace Cutlass
{
    typedef uint16_t HWindow;
    typedef uint32_t HBuffer;
    typedef uint32_t HTexture;
    //typedef uint32_t HSampler;
	typedef uint32_t HRenderPass ;
    typedef uint32_t HGraphicsPipeline;
    typedef uint32_t HCommandBuffer;

    enum class Result
    {
        eFailure = 0,
        eSuccess,
    };

    enum class ResourceType //とりあえずglmを想定している
    {
        eUint32 = 0,
        eInt32,//1
        eUnorm,//2
        eFloat32,//3
        eF32Vec2,//4
        eF32Vec3,//5
        eF32Vec4,//6
        eU32Vec2,//7
        eU32Vec3,//8
        eU32Vec4,//9
        eS32Vec2,//10
        eS32Vec3,//11
        eS32Vec4,//12
        eUNormVec2,//13
        eUNormVec3,//14
        eUNormVec4,//15
        eFMat2,//16
        eFMat3,//17
        eFMat4,//18
    };
};