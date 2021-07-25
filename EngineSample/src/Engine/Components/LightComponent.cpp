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
        //mLightCB = tmp;
    }

    void LightComponent::setAsDirectionalLight(const DirectionalLightParam& param)
    {
        auto& context = getContext();
        mType = LightType::eDirectionalLight;
        mParam = param;
        Cutlass::BufferInfo bi;
        bi.setUniformBuffer<DirectionalLightParam>();
        if(!mUB)
        {
            Cutlass::HBuffer tmp;
            context->createBuffer(bi, tmp);
            mUB = tmp;
        }

        context->writeBuffer(sizeof(DirectionalLightParam), &param, mUB.value());
    }

    const LightComponent::LightType LightComponent::getType() const
    {
        return mType;
    }

    const std::variant<LightComponent::PointLightParam, LightComponent::DirectionalLightParam>& LightComponent::getParam() const
    {
        return mParam;
    }

    const std::optional<Cutlass::HBuffer>& LightComponent::getLightUB() const
    {
        return mUB;
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

        // if(!mLightCB)
        //     return;

        // switch(mType)
        // {
        //     case LightComponent::LightType::eDirectionalLight:
        //         getContext()->writeBuffer(sizeof(DirectionalLightParam), &std::get<DirectionalLightParam>(mParam), mLightCB.value());
        //     break;
        //     case LightComponent::LightType::ePointLight:
        //         assert(!"TODO");
        //     break;
        //     default:
        //         assert(!"invalid light type!");
        //     break;
        // }
    }
}