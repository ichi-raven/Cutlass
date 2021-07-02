#pragma once

#include "Utility.hpp"

#include "Texture.hpp"

#include <vector>
#include <optional>

namespace Cutlass
{
    struct RenderPass
    {
        RenderPass() 
        {
            //do nothing(dangerous)
        };

        RenderPass
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

        RenderPass
        (
            const HTexture& colorTarget,
            const bool loadPrevData = false
        )
        : loadPrevData(loadPrevData)
        {
            colorTargets.emplace_back(colorTarget);
        }

        RenderPass
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

        RenderPass
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

        RenderPass
        (
            const std::vector<HTexture>& colorTargets,
            const bool loadPrevData = false
        )
        : colorTargets(colorTargets)
        , loadPrevData(loadPrevData)
        {

        }

        RenderPass
        (
            const std::initializer_list<HTexture>& colorTargets,
            const bool loadPrevData = false
        )
        : colorTargets(colorTargets)
        , loadPrevData(loadPrevData)
        {
            
        }

        RenderPass
        (
            const HWindow& window
        )
        : window(window)
        {

        }

        std::optional<HWindow> window;//if window handle was set, colortargets will be ignored
        std::vector<HTexture> colorTargets;
        std::optional<HTexture> depthTarget;
        bool loadPrevData;
    };
}