#include <Engine/Components/TransformComponent.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp> 

#include <chrono>

TransformComponent::TransformComponent()
{
    mWorld = glm::identity<glm::mat4>();
}

void TransformComponent::setPos(const glm::vec3& pos)
{
    mPos = pos;
}

void TransformComponent::setVel(const glm::vec3& vel)
{
    mVel = vel;
}

void TransformComponent::setAcc(const glm::vec3& acc)
{
    mAcc = acc;
}

void TransformComponent::setScale(const glm::vec3& scale)
{
    mScale = scale;
}

void TransformComponent::setRotation(const glm::vec3& rotAxis, float angle)
{
    mRotAxis = rotAxis;
    mRotAngle = angle;
}

void TransformComponent::setRotationDeg(const glm::vec3& rotAxis, float angle_deg)
{
    mRotAxis = rotAxis;
    mRotAngle = angle_deg * (M_PI / 180.f);
}



const glm::vec3& TransformComponent::getPos() const
{
    return mPos;
}

const glm::vec3& TransformComponent::getVel() const
{
    return mVel;
}

const glm::vec3& TransformComponent::getAcc() const
{
    return mAcc;
}

const glm::vec3& TransformComponent::getScale() const
{
    return mScale;
}

const glm::vec3& TransformComponent::getRotAxis() const
{
    return mRotAxis;
}

const float TransformComponent::getRotAngle() const
{
    return mRotAngle;
}

const float TransformComponent::getRotAngleDeg() const
{
    return mRotAngle / M_PI * 180.f;
}

const glm::mat4& TransformComponent::getWorldMatrix()
{
    return mWorld;
}

void TransformComponent::update()
{
    auto&& now = std::chrono::high_resolution_clock::now();
    float&& deltatime = std::chrono::duration_cast<std::chrono::microseconds>(now - mPrev).count() / 1000000.f;
    mPos += deltatime * (mVel += deltatime * mAcc);

    mWorld = glm::translate(glm::mat4(), mPos) * glm::rotate(mRotAngle, mRotAxis) * glm::scale(mScale); 

    mPrev = now;
}