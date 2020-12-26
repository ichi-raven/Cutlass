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

        void init(Cutlass::Context& context, const uint32_t width, const uint32_t height, const uint32_t frameCount, const Cutlass::HRenderDST& windowRDST);

        //Noncopyable, Nonmoveable
        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer &operator=(const BaseRenderer&) = delete;
        BaseRenderer(BaseRenderer&&) = delete;
        BaseRenderer &operator=(BaseRenderer&&) = delete;

        virtual void addMesh
        (
            const std::shared_ptr<MeshComponent> mesh, 
            const std::shared_ptr<MaterialComponent> material
        );

        virtual void addCamera
        (
            const std::shared_ptr<CameraComponent> camera
        );

        virtual void addLight
        (
            const std::shared_ptr<LightComponent> light
        );

        virtual void render(Cutlass::Context& context);

    private:
        struct MVP
        {
            glm::mat4 world;
            glm::mat4 view;
            glm::mat4 proj;
        };

        uint32_t mWidth;
        uint32_t mHeight;
        uint32_t mFrameCount;
        Cutlass::HRenderDST mWindowRDST;

        Cutlass::HTexture mRTTex;
        Cutlass::HRenderDST mIntermediateDST;

        using MeshWithMaterial = std::pair<std::shared_ptr<MeshComponent>, std::shared_ptr<MaterialComponent>>;
        std::queue<MeshWithMaterial> mMeshWithMaterialQueue;
        std::shared_ptr<CameraComponent> mCamera;
        std::queue<std::shared_ptr<LightComponent>> mLightQueue;

        std::vector<Cutlass::HBuffer> mMVPCBs;
        std::vector<Cutlass::HBuffer> mSceneCBs;

        //描画パス
        Cutlass::RenderPipelineInfo mR2TPassInfo;
        Cutlass::RenderPipelineInfo mPresentPassInfo;
    };
};