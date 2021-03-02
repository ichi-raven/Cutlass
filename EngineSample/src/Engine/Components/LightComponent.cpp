#include <Engine/Components/LightComponent.hpp>

namespace Engine
{
    LightComponent::LightComponent()
    {
        mEnable = true;

    }

    void LightComponent::setAsPointLight(const PointLightParam& param)
    {

    }

    void LightComponent::setAsDirectionalLight(const DirectionalLightParam& param)
    {

    }

    void LightComponent::setEnable(bool flag)
    {
        mEnable = flag;
    }

    void LightComponent::setEnable()
    {
        mEnable = !mEnable;
    }

    const bool LightComponent::getEnable() const
    {
        return mEnable;
    }

    void LightComponent::setTransform(const Transform& transform)
    {
        mTransform = transform;
    }

    Transform& LightComponent::getTransform()
    {
        return mTransform;
    }

    const Transform& LightComponent::getTransform() const
    {
        return mTransform;
    }

    void LightComponent::update()
    {
        mTransform.update();
    }
}