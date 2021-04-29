#include <Engine/Components/CameraComponent.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>//debug

namespace Engine
{
    CameraComponent::CameraComponent()
    {
        mEnable = true;
        mLookPos = glm::vec3(0, 0, 10.f);
        mUp = glm::vec3(0, 1.f, 0);
        mFovY = glm::radians(45.f);
        mAspect = 640.f / 480.f;//適当、設定すべきである
        mNear = 0.1f;
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

    void CameraComponent::setProjectionParam(float fovAngle, uint32_t width, uint32_t height, float near, float far)
    {
        mFovY = glm::radians(fovAngle);
        setAspectAuto(width, height);
        mNear = near;
        mFar = far;
    }

    void CameraComponent::setFovY(float fovAngle)
    {
        mFovY = fovAngle;
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
        mProjection[1][1] *= -1;

        // std::cout << "view : \n";
        // for(int i = 0; i < 4; ++i)
        // {
        //     for(int j = 0; j < 4; ++j)
        //         std::cout << mView[i][j] << " ";
        //     std::cout << "\n";
        // }

        // std::cout << "projection : \n";
        // for(int i = 0; i < 4; ++i)
        // {
        //     for(int j = 0; j < 4; ++j)
        //         std::cout << mProjection[i][j] << " ";
        //     std::cout << "\n";
        // }
    }

}