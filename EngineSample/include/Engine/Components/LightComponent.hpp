#pragma once

#include "IComponent.hpp"
#include "../Utility/Transform.hpp"

#include <Cutlass.hpp>

#include <variant>

namespace Engine
{
   //実装が適当、まだ多分使用不能
    class LightComponent : public IComponent
    {
    public:
        enum class LightType
        {
            //いろいろ
            ePointLight, 
            eDirectionalLight,
        };
        
        struct PointLightParam
        {
            glm::vec4 lightColor;
        };

        struct DirectionalLightParam
        {
            glm::vec4 lightDir;
            glm::vec4 lightColor; 
        };

        LightComponent();
 
        virtual ~LightComponent(){}

        void setAsPointLight(const PointLightParam& param);

        void setAsDirectionalLight(const DirectionalLightParam& param);

        void setTransform(const Transform& transform);
        Transform& getTransform();
        const Transform& getTransform() const;

        const LightType getType() const;
        const std::optional<Cutlass::HBuffer>& getLightCB() const;

        void setEnable(bool flag);
        void setEnable();
        const bool getEnable() const;

        virtual void update() override;

    private:
        bool mEnable;
        Transform mTransform;
        std::optional<Cutlass::HBuffer> mLightCB;
        LightType mType;
        std::variant<PointLightParam, DirectionalLightParam> mParam;
    };
}