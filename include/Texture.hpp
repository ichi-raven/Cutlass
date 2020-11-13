#pragma once

#include <cstdint>
#include <utility>
#include "Utility.hpp"

namespace Cutlass
{

    enum class TextureUsage
    {
        eShaderResource,
        eColorTarget,
        eDepthStencilTarget,
        eSwapchainImage,//自分で指定しても破損するだけです
        //eUnordered,
    };

    enum class Dimention
    {
        e2D,
        //e3D, //現段階ではまだ未定
    };

    // enum class FormatType
    // {
    //     eUNorm,
    //     eFloat,
    // };

	enum class SamplerType
	{
		eDefault,
	};

    struct TextureInfo
    {
        TextureUsage usage;
        Dimention dimention;
        ResourceType format;
		SamplerType samplerType;
        bool isHostVisible;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };
};