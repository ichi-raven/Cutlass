#include <Engine/Components/LightComponent.hpp>

namespace Engine
{
    LightComponent::LightComponent()
    : mEnable(true)
    {
        //とりあえず
        // DirectionalLightParam data;
        mColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.f);
        mDirection = glm::vec3(0);//glm::vec3(0, 0.7071f, 0.7071f);
        //setAsDirectionalLight(data);
    }

    //void LightComponent::setAsPointLight(const PointLightParam& param)
    // {
    //     assert(!"TODO");
    //     mType = LightType::ePointLight;
    //     mParam = param;
    //     // Cutlass::BufferInfo bi;
    //     // bi.setUniformBuffer<PointLightParam>();
    //     // Cutlass::HBuffer tmp;
    //     // getContext()->createBuffer(bi, tmp);
    //     //mLightCB = tmp;
    // }

    // void LightComponent::setAsDirectionalLight(const DirectionalLightParam& param)
    // {
    //     //auto& context = getContext();
    //     mType = LightType::eDirectionalLight;
    //     mParam = param;
    //     // Cutlass::BufferInfo bi;
    //     // bi.setUniformBuffer<DirectionalLightParam>();
    //     // if(!mUB)
    //     // {
    //     //     Cutlass::HBuffer tmp;
    //     //     context->createBuffer(bi, tmp);
    //     //     mUB = tmp;
    //     // }

    //     //context->writeBuffer(sizeof(DirectionalLightParam), &param, mUB.value());
    // }

    void LightComponent::setAsPointLight(const glm::vec4& color)
    {
        mType = LightType::ePointLight;
        mColor = color;
        //mDirection = glm::vec3(0);
    }

    void LightComponent::setAsDirectionalLight(const glm::vec4& color, const glm::vec3& direction)
    {
        mType = LightType::eDirectionalLight;
        mColor = color;
        mDirection = glm::normalize(direction);
    }

    const LightComponent::LightType LightComponent::getType() const
    {
        return mType;
    }

    const glm::vec4& LightComponent::getColor() const
    {
        return mColor;
    }

    const glm::vec3& LightComponent::getDirection() const
    {
        return mDirection;
    }

    // const std::variant<LightComponent::PointLightParam, LightComponent::DirectionalLightParam>& LightComponent::getParam() const
    // {
    //     return mParam;
    // }

    //const std::optional<Cutlass::HBuffer>& LightComponent::getLightUB() const
    // {
    //     return mUB;
    // }

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