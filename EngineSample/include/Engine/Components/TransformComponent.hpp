#pragma once

#include <chrono>

#include <glm/glm.hpp>

#include "IComponent.hpp"

class TransformComponent : public IComponent
{
public:
    TransformComponent();

    void setPos(const glm::vec3& pos);
    void setVel(const glm::vec3& vel);
    void setAcc(const glm::vec3& acc);
    void setScale(const glm::vec3& scale);
    void setRotation(const glm::vec3& rotAxis, float angle);
    void setRotationDeg(const glm::vec3& rotAxis, float angle_deg);

    const glm::vec3& getPos() const;
    const glm::vec3& getVel() const;
    const glm::vec3& getAcc() const;
    const glm::vec3& getScale() const;
    const glm::vec3& getRotAxis() const;
    const float getRotAngle() const;
    const float getRotAngleDeg() const;


    const glm::mat4& getWorldMatrix();

    virtual void update();

private:
    glm::vec3 mPos;
    glm::vec3 mVel;
    glm::vec3 mAcc;

    glm::vec3 mScale;

    glm::vec3 mRotAxis;
    float mRotAngle;//rad

    glm::mat4 mWorld;

    std::chrono::high_resolution_clock::time_point mPrev;
};