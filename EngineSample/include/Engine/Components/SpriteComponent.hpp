#pragma once 

#include "../Components/IComponent.hpp"

#include "../Utility/Transform.hpp"

#include <Cutlass.hpp>

namespace Engine
{
    class SpriteComponent : public IComponent
    {
    public:

        struct Sprite
        {
            std::vector<Cutlass::HTexture> handles;
        };

        SpriteComponent();
        virtual ~SpriteComponent();

        void setTransform(const Transform& transform);

        Transform& getTransform();

        void create(const Sprite& sprite);

        const Cutlass::HTexture& getSpriteTexture() const;

        virtual void update();

    private:
        Transform mTransform;
        Sprite mSprite;
        size_t mIndex;
    };
}