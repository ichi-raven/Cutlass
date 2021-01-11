#pragma once

#include "Utility.hpp"

#include "Texture.hpp"

#include <vector>
#include <optional>

namespace Cutlass
{
    struct RenderPassCreateInfo
    {
        RenderPassCreateInfo
        (
            const HTexture& colorTarget,
            const HTexture& depthTarget,
            const bool loadPrevData = false
        )
        : depthTarget(depthTarget)
        , loadPrevData(loadPrevData)
        {
            colorTargets.emplace_back(colorTarget);
        }

        RenderPassCreateInfo
        (
            const HTexture& colorTarget,
            const bool loadPrevData = false
        )
        : loadPrevData(loadPrevData)
        {
            colorTargets.emplace_back(colorTarget);
        }

        RenderPassCreateInfo
        (
            const std::vector<HTexture>& colorTargets,
            const HTexture& depthTarget,
            const bool loadPrevData = false
        )
        : colorTargets(colorTargets)
        , depthTarget(depthTarget)
        , loadPrevData(loadPrevData)
        {

        }

        RenderPassCreateInfo
        (
            const std::initializer_list<HTexture>& colorTargets,
            const HTexture& depthTarget,
            const bool loadPrevData = false
        )
        : colorTargets(colorTargets)
        , depthTarget(depthTarget)
        , loadPrevData(loadPrevData)
        {

        }

        RenderPassCreateInfo
        (
            const std::vector<HTexture>& colorTargets,
            const bool loadPrevData = false
        )
        : colorTargets(colorTargets)
        , loadPrevData(loadPrevData)
        {

        }

        RenderPassCreateInfo
        (
            const std::initializer_list<HTexture>& colorTargets,
            const bool loadPrevData = false
        )
        : colorTargets(colorTargets)
        , loadPrevData(loadPrevData)
        {
            
        }

        std::vector<HTexture> colorTargets;
        std::optional<HTexture> depthTarget;
        bool loadPrevData;
    };
}