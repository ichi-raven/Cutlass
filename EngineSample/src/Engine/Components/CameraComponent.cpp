#include <Engine/Components/CameraComponent.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine
{
    CameraComponent::CameraComponent()
    {
        mEnable = true;
        mLookPos = glm::vec3(0, -10.f, 0);
        mUp = glm::vec3(0, 1.f, 0);
        mFovY = 60.f;
        mAspect = 640.f / 480.f;
        mNear = 1.f;
        mFar = static_cast<float>(1e6);
    }

    void CameraComponent::setEnable(bool flag)
    {
        mEnable = flag;
    }

    void CameraComponent::setEnable()
    {
        mEnable = !mEnable;
    }

    const bool CameraComponent::getEnable() const
    {
        return mEnable;
    }

    void CameraComponent::setTransform(const Transform& transform)
    {
        mTransform = transform;
    }

    Transform& CameraComponent::getTransform()
    {
        return mTransform;
    }

    const Transform& CameraComponent::getTransform() const
    {
        return mTransform;
    }

    void CameraComponent::setViewParam(const glm::vec3& lookPos, const glm::vec3& up)
    {
        mLookPos = lookPos;
        mUp = up;
    }

    void CameraComponent::setLookAt(const glm::vec3& lookPos)
    {
        mLookPos = lookPos;
    }

    const glm::vec3& CameraComponent::getLookAt() const
    {
        return mLookPos;
    }

    void CameraComponent::setUpDir(const glm::vec3& up)
    {
        mUp = up;
    }

    const glm::vec3& CameraComponent::getUpDir() const
    {
        return mUp;
    }

    void CameraComponent::setProjectionParam(float fovy, uint32_t width, uint32_t height, float near, float far)
    {
        mFovY = fovy;
        setAspectAuto(width, height);
        mNear = near;
        mFar = far;
    }

    void CameraComponent::setFovY(float fovy)
    {
        mFovY = fovy;
    }

    void CameraComponent::setFovY_deg(float fovy_deg)
    {
        mFovY = fovy_deg * (M_PI / 180.f);
    }

    const float CameraComponent::getFovY() const
    {
        return mFovY;
    }

    void CameraComponent::setAspect(float aspect)
    {
        mAspect = aspect;
    }

    void CameraComponent::setAspectAuto(uint32_t screenWidth, uint32_t screenHeight)
    {
        mAspect = 1.f * screenWidth / screenHeight;
    }

    const float CameraComponent::getAspect() const
    {
        return mAspect;
    }

    void CameraComponent::setNearFar(float near, float far)
    {
        mNear = near;
        mFar = far;
    }

    const float CameraComponent::getNear() const
    {
        return mNear;
    }

    const float CameraComponent::getFar() const
    {
        return mFar;
    }

    const glm::mat4& CameraComponent::getViewMatrix() const
    {
        return mView;
    }

    const glm::mat4& CameraComponent::getProjectionMatrix() const
    {
        return mProjection;
    }

    void CameraComponent::update()
    {
        mTransform.update();
        mView = glm::lookAtRH(mTransform.getPos(), mLookPos, mUp);
        mProjection = glm::perspective(mFovY, mAspect, mNear, mFar);
    }

}