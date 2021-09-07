#include <Engine/Components/SpriteComponent.hpp>

namespace Engine
{
    SpriteComponent::SpriteComponent()
    : mIndex(0)
    {

    }

    SpriteComponent::~SpriteComponent()
    {

    }

    void SpriteComponent::setTransform(const Transform& transform)
    {
        mTransform = transform;
    }

    Transform& SpriteComponent::getTransform()
    {
        return mTransform;
    }

    void SpriteComponent::create(const Sprite& sprite)
    {
        mSprite = sprite;
    }

    const Cutlass::HTexture& SpriteComponent::getSpriteTexture() const
    {
        if(mSprite.handles.size() == 0)
            assert(!"sprite is not created!");

        return mSprite.handles[mIndex];
    }

    void SpriteComponent::update()
    {
        mTransform.update();
        mIndex = (1 + mIndex) % mSprite.handles.size();
        // if(mTransform.getPos().z != 0)
        // {
        //     assert(!"Sprite's Z is not zero!");
        // }
    }

}