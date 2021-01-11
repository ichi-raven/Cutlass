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
        eUnordered,
        eSwapchainImage,//自分で指定しても破損するだけです
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

        TextureInfo(uint32_t width, uint32_t height, uint32_t depth = 1u, ResourceType format = ResourceType::eUNormVec4, TextureUsage usage = TextureUsage::eUnordered, bool isHostVisible = true, Dimension dimension = Dimension::e2D, SamplerType samplerType = SamplerType::eDefault)
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

        inline void setSRTex2D(uint32_t _width, uint32_t _height, bool _isHostVisible, ResourceType _format = ResourceType::eUNormVec4, SamplerType _samplerType = SamplerType::eDefault)
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

        inline void setRTTex2D(uint32_t _width, uint32_t _height, ResourceType _format = ResourceType::eUNormVec4, bool _isHostVisible = true, SamplerType _samplerType = SamplerType::eDefault)
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

        inline void setRTTex2DColor(uint32_t _width, uint32_t _height, ResourceType _format = ResourceType::eUNormVec4, bool _isHostVisible = true, SamplerType _samplerType = SamplerType::eDefault)
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

        inline void setRTTex2DDepth(uint32_t _width, uint32_t _height, ResourceType _format = ResourceType::eFloat32, bool _isHostVisible = true, SamplerType _samplerType = SamplerType::eDefault)
        {
            width = _width;
            height = _height;
            depth = 1;
            dimension = Dimension::e2D;
            format = _format;
            isHostVisible = _isHostVisible;
            format = format;
            samplerType = _samplerType;
            usage = TextureUsage::eDepthStencilTarget;
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