#pragma once

#include <Cutlass.hpp>

#include <memory>
#include <cstdint>

#include <glm/glm.hpp>

namespace Engine
{
    class Input
    {
    public:
        Input(const std::shared_ptr<Cutlass::Context>& context)
        : mContext(context)
        {

        }

        uint32_t getKey(Cutlass::Key key);

        bool getKeyDown(Cutlass::Key key);

        glm::vec2 getMousePos();

    private:
        std::shared_ptr<Cutlass::Context> mContext;
    };
}