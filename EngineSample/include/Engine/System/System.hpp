#pragma once

#include <memory>

#include "Loader.hpp"
#include "Renderer.hpp"

namespace Engine
{
    struct System
    {
        std::unique_ptr<Loader> mLoader;
        std::unique_ptr<Renderer> mRenderer;
    };
}