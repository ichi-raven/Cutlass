#pragma once

#include "IComponent.hpp"
#include "../Utility/Transform.hpp"

namespace Engine
{
    enum class LightType
    {
        //いろいろ
        ePointLight, 
        eDirectionalLight,
    };
    
    struct PointLightParam
    {

    };

    struct DirectionalLightParam
    {
        
    };

    //実装が適当、まだ多分使用不能
    class LightComponent : public IComponent
    {
    public:
        LightComponent();

        void setAsPointLight(const PointLightParam& param);

        void setAsDirectionalLight(const DirectionalLightParam& param);

        void setTransform(const Transform& transform);
        Transform& getTransform();
        const Transform& getTransform() const;

        void setEnable(bool flag);
        void setEnable();
        const bool getEnable() const;

        virtual void update() override;

    private:
        bool mEnable;
        Transform mTransform;
    };
}