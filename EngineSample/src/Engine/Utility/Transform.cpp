#include <Engine/Utility/Transform.hpp>

#include <glm/gtx/transform.hpp>
//#include <glm/gtc/matrix_transform.hpp> 

#include <chrono>
#include <iostream>

namespace Engine
{

    Transform::Transform()
    : mWorld(glm::identity<glm::mat4>())
    , mPos(0)
    , mVel(0)
    , mAcc(0)
    , mRotAngle(0)
    , mRotVel(0)
    , mRotAcc(0)
    , mRotAxis(glm::vec3(0, 1.f, 0))
    , mScale(1.f)
    , mPrev(std::chrono::high_resolution_clock::now())
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

    void Transform::setRotation(float angle)
    {
        mRotAngle = angle;
    }

    void Transform::setRotAxis(const glm::vec3& rotAxis)
    {
        mRotAxis = rotAxis;
    }

    void Transform::setRotVel(const glm::vec3& rotAxis, float angleVel)
    {
        mRotAxis = rotAxis;
        mRotVel = angleVel;
    }

    void Transform::setRotVel(float angleVel)
    {
        mRotVel = angleVel;
    }

    void Transform::setRotAcc(const glm::vec3& rotAxis, float angleAcc)
    {
        mRotAxis = rotAxis;
        mRotAcc = angleAcc;
    }

    void Transform::setRotAcc(float angleAcc)
    {
        mRotAcc = angleAcc;
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

    const float Transform::getRotVel() const
    {
        return mRotVel;
    }

    const glm::mat4& Transform::getWorldMatrix()
    {
        return mWorld;
    }

    void Transform::update()
    {
        static float deltaTime = 0;
        auto&& now = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - mPrev).count() / 1000000.;
        mPos += deltaTime * (mVel += (deltaTime * mAcc));
        mRotAngle += deltaTime * (mRotVel += (deltaTime * mRotAcc));
        //std::cout << "transform pos : " << mPos.x << "," << mPos.y << "," << mPos.z << "\n";

        mWorld = glm::translate(glm::mat4(1.f), mPos) * glm::rotate(mRotAngle, mRotAxis) * glm::scale(mScale); 
        mPrev = now;
        
        // for(int i = 0; i < 4; ++i)
        // {
        //     for(int j = 0; j < 4; ++j)
        //         std::cout << mWorld[i][j] << " ";
        //     std::cout << "\n";
        // }

    }

    Transform::~Transform()
    {
    
    }
}