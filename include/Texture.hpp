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

    enum class Dimension
    {
        e2D,
        //e3D, //現段階ではまだ未定
    };

	enum class SamplerType
	{
		eDefault,
	};

    struct TextureInfo
    {
        TextureInfo() {}

        TextureInfo(uint32_t width, uint32_t height, uint32_t depth, TextureUsage usage, Dimension dimension, ResourceType format, SamplerType samplerType, bool isHostVisible)
            : width(width)
            , height(height)
            , depth(depth)
            , usage(usage)
            , dimension(dimension)
            , format(format)
            , samplerType(samplerType)
            , isHostVisible(isHostVisible)
        {

        }

        inline void setSRTex2D(uint32_t _width, uint32_t _height, bool _isHostVisible, ResourceType _format = ResourceType::eF32Vec4, SamplerType _samplerType = SamplerType::eDefault)
        {
            width = _width;
            height = _height;
            depth = 1;
            dimension = Dimension::e2D;
            format = _format;
            isHostVisible = _isHostVisible;
            format = format;
            samplerType = _samplerType;
            usage = TextureUsage::eShaderResource;
        }

        inline void setRTTex2D(uint32_t _width, uint32_t _height, ResourceType _format = ResourceType::eF32Vec4, bool _isHostVisible = true, SamplerType _samplerType = SamplerType::eDefault)
        {
            width = _width;
            height = _height;
            depth = 1;
            dimension = Dimension::e2D;
            format = _format;
            isHostVisible = _isHostVisible;
            format = format;
            samplerType = _samplerType;
            usage = TextureUsage::eColorTarget;
        }

        TextureUsage usage;
        Dimension dimension;
        ResourceType format;
		SamplerType samplerType;
        bool isHostVisible;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };
};