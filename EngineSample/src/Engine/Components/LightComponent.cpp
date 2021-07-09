#include <Engine/Components/LightComponent.hpp>

namespace Engine
{
    LightComponent::LightComponent()
    : mEnable(true)
    {
        DirectionalLightParam data;
        data.lightDir = glm::vec4(1.f, 1.f, 1.f, 0);
        data.lightColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        //setAsDirectionalLight(data);
    }

    void LightComponent::setAsPointLight(const PointLightParam& param)
    {
        assert(!"TODO");
        mType = LightType::ePointLight;
        mParam = param;
        Cutlass::BufferInfo bi;
        bi.setUniformBuffer<PointLightParam>();
        Cutlass::HBuffer tmp;
        getContext()->createBuffer(bi, tmp);
        mLightCB = tmp;
    }

    void LightComponent::setAsDirectionalLight(const DirectionalLightParam& param)
    {
        auto& context = getContext();
        mType = LightType::eDirectionalLight;
        mParam = param;
        Cutlass::BufferInfo bi;
        bi.setUniformBuffer<DirectionalLightParam>();
        Cutlass::HBuffer tmp;
        context->createBuffer(bi, tmp);
        //assert(0);
        mLightCB = tmp;
    }

    const LightComponent::LightType LightComponent::getType() const
    {
        return mType;
    }

    const std::optional<Cutlass::HBuffer>& LightComponent::getLightCB() const
    {
        return mLightCB;
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