#pragma once

#include "Loader.hpp"
#include "Renderer.hpp"

#include <memory>

namespace Engine
{
    //新しいSystemはここに追加しよう
    struct System
    {
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<Loader> loader;
    };
}