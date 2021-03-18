#pragma once

#include "IComponent.hpp"

namespace Engine
{
    class EffectComponent : public IComponent
    {
    public:
        virtual ~EffectComponent(){}
        virtual void update();
    private:

    };
}