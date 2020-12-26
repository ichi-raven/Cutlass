#pragma once

#include "IComponent.hpp"

namespace Engine
{
    enum class LightType
    {
        //いろいろ
        ePointLight, 
        eDirectionalLight,
    };
    
    //実装が適当、まだ多分使用不能
    class LightComponent : public IComponent
    {
    public:
        void setAsPointLight();

        void setAsDirectionalLight();

        void setTransform(const Transform& transform);
        Transform& getTransform();
        const Transform& getTransform() const;

        void setEnable(bool flag);
        bool getEnable() const;

        virtual void update() override;

    private:
        bool mEnable;


    };
}