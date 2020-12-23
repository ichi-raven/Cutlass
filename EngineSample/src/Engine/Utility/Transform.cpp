#include <Engine/Utility/Transform.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp> 

#include <chrono>

namespace Engine
{
    Transform::Transform()
    : mWorld(glm::identity<glm::mat4>())
    , mRotAngle(0)
    {

    }

    void Transform::setPos(const glm::vec3& pos)
    {
        mPos = pos;
    }

    void Transform::setVel(const glm::vec3& vel)
    {
        mVel = vel;
    }

    void Transform::setAcc(const glm::vec3& acc)
    {
        mAcc = acc;
    }

    void Transform::setScale(const glm::vec3& scale)
    {
        mScale = scale;
    }

    void Transform::setRotation(const glm::vec3& rotAxis, float angle)
    {
        mRotAxis = rotAxis;
        mRotAngle = angle;
    }

    void Transform::setRotationDeg(const glm::vec3& rotAxis, float angle_deg)
    {
        mRotAxis = rotAxis;
        mRotAngle = angle_deg * (M_PI / 180.f);
    }


    const glm::vec3& Transform::getPos() const
    {
        return mPos;
    }

    const glm::vec3& Transform::getVel() const
    {
        return mVel;
    }

    const glm::vec3& Transform::getAcc() const
    {
        return mAcc;
    }

    const glm::vec3& Transform::getScale() const
    {
        return mScale;
    }

    const glm::vec3& Transform::getRotAxis() const
    {
        return mRotAxis;
    }

    const float Transform::getRotAngle() const
    {
        return mRotAngle;
    }

    const float Transform::getRotAngleDeg() const
    {
        return mRotAngle / M_PI * 180.f;
    }

    const glm::mat4& Transform::getWorldMatrix()
    {
        return mWorld;
    }

    void Transform::update()
    {
        auto&& now = std::chrono::high_resolution_clock::now();
        float&& deltatime = std::chrono::duration_cast<std::chrono::microseconds>(now - mPrev).count() / 1000000.f;
        mPos += deltatime * (mVel += deltatime * mAcc);

        mWorld = glm::translate(glm::mat4(), mPos) * glm::rotate(mRotAngle, mRotAxis) * glm::scale(mScale); 

        mPrev = now;
    }
}