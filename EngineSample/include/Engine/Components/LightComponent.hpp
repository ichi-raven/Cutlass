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

        LightComponent();
 
        virtual ~LightComponent(){}

        void setAsPointLight(const glm::vec4& color);

        void setAsDirectionalLight(const glm::vec4& color, const glm::vec3& direction);

        void setTransform(const Transform& transform);
        Transform& getTransform();
        const Transform& getTransform() const;

        const LightType getType() const;

        const glm::vec4& getColor() const;
        const glm::vec3& getDirection() const;

        //const std::optional<Cutlass::HBuffer>& getLightUB() const;


        void setEnable(bool flag);
        void setEnable();
        const bool getEnable() const;

        virtual void update() override;

    private:
        bool mEnable;
        Transform mTransform;
        LightType mType;
        
        glm::vec4 mColor;
        glm::vec3 mDirection;

        //std::optional<Cutlass::HBuffer> mUB;
    };
}