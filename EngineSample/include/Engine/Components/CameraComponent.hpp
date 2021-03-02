#pragma once

#include "IComponent.hpp"

#include "../Utility/Transform.hpp"

namespace Engine
{
    class CameraComponent : public IComponent
    {
    public:
        CameraComponent();

        void setEnable(bool flag);
        void setEnable();
        const bool getEnable() const;

        void setTransform(const Transform& transform);
        Transform& getTransform();
        const Transform& getTransform() const;

        void setLookAt(const glm::vec3& lookPos);
        const glm::vec3& getLookAt() const;

        void setUpDir(const glm::vec3& up = glm::vec3(0, 1.f, 0));
        const glm::vec3& getUpDir() const;

        void setViewParam(const glm::vec3& lookPos, const glm::vec3& up = glm::vec3(0, 1.f, 0));

        void setProjectionParam(float fovy, uint32_t width, uint32_t height, float near, float far);

        void setFovY(float fovy);
        void setFovY_deg(float fovy_deg);
        const float getFovY() const;

        void setAspect(float aspect);
        void setAspectAuto(uint32_t width, uint32_t height);
        const float getAspect() const;
        void setNearFar(float near, float far);
        const float getNear() const;
        const float getFar() const;

        const glm::mat4& getViewMatrix() const;
        const glm::mat4& getProjectionMatrix() const;

        virtual void update();

    private:
        bool mEnable;

        Transform mTransform;

        glm::vec3 mLookPos;
        glm::vec3 mUp;
        float mFovY;
        float mAspect;
        float mNear;
        float mFar;

        glm::mat4 mView;
        glm::mat4 mProjection;
    };
};