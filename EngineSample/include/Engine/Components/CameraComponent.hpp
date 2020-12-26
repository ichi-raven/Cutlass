#pragma once

#include "IComponent.hpp"

#include "../Utility/Transform.hpp"

namespace Engine
{
    class CameraComponent : public IComponent
    {
    public:
        void setEnable(bool flag);
        bool getEnable() const;

        void setTransform(const Transform& transform);
        Transform& getTransform();
        const Transform& getTransform() const;

        void setLookAt(glm::vec3 lookPos);
        const glm::vec3& getLookAt() const;

        void setUpDir(glm::vec3 up);
        const glm::vec3& getupDir();

        const glm::mat4& getViewMatrix() const;

        virtual void update();

    private:
        Transform mTransform;

        glm::mat4 mView;

    };
};