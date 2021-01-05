#pragma once

#include "Utility.hpp"

#include "Texture.hpp"

#include <vector>
#include <optional>

namespace Cutlass
{
    //当然ながら, initialUsageを指定した場合はクリアは実行されません
    struct RenderPassCreateInfo
    {
        RenderPassCreateInfo
        (
            const HTexture& colorTarget,
            const HTexture& depthTarget,
            const TextureUsage& initialUsage
        )
        : depthTarget(depthTarget)
        , initialUsage(initialUsage)
        {
            colorTargets.emplace_back(colorTarget);
        }

        RenderPassCreateInfo
        (
            const HTexture& colorTarget,
            const HTexture& depthTarget
        )
        : depthTarget(depthTarget)
        {
            colorTargets.emplace_back(colorTarget);
        }

        RenderPassCreateInfo
        (
            const HTexture& colorTarget,
            const TextureUsage& initialUsage
        )
        : initialUsage(initialUsage)
        {
            colorTargets.emplace_back(colorTarget);
        }

        RenderPassCreateInfo
        (
            const HTexture& colorTarget
        )
        {
            colorTargets.emplace_back(colorTarget);
        }

        RenderPassCreateInfo
        (
            const std::vector<HTexture>& colorTargets,
            const HTexture& depthTarget,
            const TextureUsage& initialUsage
        )
        : colorTargets(colorTargets)
        , depthTarget(depthTarget)
        , initialUsage(initialUsage)
        {

        }

        RenderPassCreateInfo
        (
            const std::initializer_list<HTexture>& colorTargets,
            const HTexture& depthTarget
        )
        : colorTargets(colorTargets)
        , depthTarget(depthTarget)
        {

        }

        RenderPassCreateInfo
        (
            const std::vector<HTexture>& colorTargets,
            const TextureUsage& initialUsage
        )
        : colorTargets(colorTargets)
        , initialUsage(initialUsage)
        {

        }

        RenderPassCreateInfo
        (
            const std::initializer_list<HTexture>& colorTargets
        )
        : colorTargets(colorTargets)
        {

        }

        std::vector<HTexture> colorTargets;
        std::optional<HTexture> depthTarget;
        std::optional<TextureUsage> initialUsage;
    };
}