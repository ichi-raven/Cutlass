#pragma once

#include <Cutlass.hpp>
#include <queue>

#include <glm/glm.hpp>

namespace Engine
{
    class MeshComponent;
    class MaterialComponent;
    class CameraComponent;
    class LightComponent;

    //本エンジンはエンジンとして描画を担当する気がありません
    //これは実装の一例です これを適当に継承して適当に作っていただくか自作していただければ
    class BaseRenderer
    {
    public:
        BaseRenderer(){}

        //Noncopyable, Nonmoveable
        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer &operator=(const BaseRenderer&) = delete;
        BaseRenderer(BaseRenderer&&) = delete;
        BaseRenderer &operator=(BaseRenderer&&) = delete;

        void init(Cutlass::Context& context, const uint32_t width, const uint32_t height, const uint32_t frameCount, const Cutlass::HWindow& window);

    private:
        
    };
};