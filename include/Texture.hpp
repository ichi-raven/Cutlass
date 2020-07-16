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

    enum class TextureDimention
    {
        e2D,
        //e3D, //現段階ではまだ未定
    };

    enum class TextureFormatType
    {
        eUNorm,
        eFloat,
    };

	enum class SamplerType
	{
		eDefault,
	};

    struct TextureInfo
    {
        TextureUsage usage;
        TextureDimention dimention;
        std::pair<ResourceType, TextureFormatType> format;
		SamplerType samplerType;
        bool isHostVisible;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };
};