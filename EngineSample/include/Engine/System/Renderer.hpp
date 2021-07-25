#pragma once

#include <Cutlass.hpp>

#include <glm/glm.hpp>

#include <vector>

#include "../Actors/IActor.hpp"

namespace Engine
{
    class MeshComponent;
    class SkeltalMeshComponent;
    class MaterialComponent;
    class CustomMaterialComponent;
    class LightComponent;
    class CameraComponent;

    class Renderer
    {
    public:
        Renderer() = delete;

        Renderer(std::shared_ptr<Cutlass::Context> context, const std::vector<Cutlass::HWindow>& hwindows, const uint16_t frameCount = 3);

        //Noncopyable, Nonmoveable
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        virtual ~Renderer();

        virtual void regist(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<MaterialComponent>& material);
        virtual void regist(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<CustomMaterialComponent>& material);
        virtual void regist(const std::shared_ptr<SkeltalMeshComponent>& mesh, const std::shared_ptr<CustomMaterialComponent>& material);

        virtual void addLight(const std::shared_ptr<LightComponent>& light);

        virtual void clearScene();//Scene変更時に情報をアンロードする

        //TODO
        //virtual void addPostEffect();

        //このカメラになる
        virtual void setCamera(const std::shared_ptr<CameraComponent>& camera);

        //現在設定されている情報から描画用シーンをビルドする
        virtual void build();

        //描画コマンド実行
        virtual void render();

    protected:
        std::shared_ptr<Cutlass::Context> mContext;
        std::vector<Cutlass::HWindow> mHWindows;

    private:
        struct GBuffer//G-Buffer
        {
            Cutlass::HTexture albedoRT;
            Cutlass::HTexture normalRT;
            Cutlass::HTexture worldPosRT;
            Cutlass::HTexture depthBuffer;

            Cutlass::HRenderPass renderPass;
        };

        struct SceneData
        {
            glm::mat4 world;
            glm::mat4 view;
            glm::mat4 proj;
        };

        struct CameraData
        {
            glm::vec3 cameraPos;
        };

        //#define MAXLIGHTNUM (64)
        struct LightData//固定長64個まで
        {
            glm::vec4 lightDir;
            glm::vec4 lightColor;
            // glm::vec4 lightDir[MAXLIGHTNUM];
            // glm::vec4 lightColor[MAXLIGHTNUM];
            //uint32_t lightNum;
        };

        struct RenderInfo
        {
            std::shared_ptr<MeshComponent> mesh;
            std::shared_ptr<MaterialComponent> material;
            
            std::optional<Cutlass::HBuffer> sceneCB;
            
            Cutlass::HGraphicsPipeline pipeline;
        };

        const uint16_t mFrameCount;

        std::vector<Cutlass::HTexture> mDepthBuffers;

        std::vector<Cutlass::HTexture> mRTTexs;
        std::vector<std::pair<Cutlass::HRenderPass, Cutlass::HGraphicsPipeline>> mPresentPasses;

        GBuffer mGBuffer;
        Cutlass::Shader mShadowVS;
        Cutlass::Shader mShadowPS;
        Cutlass::Shader mDefferedVS;
        Cutlass::Shader mDefferedFS;
        Cutlass::Shader mDefferedSkinVS;
        Cutlass::Shader mDefferedSkinFS;
        Cutlass::Shader mLightingVS;
        Cutlass::Shader mLightingFS;

        Cutlass::HRenderPass mLightingPass;
        Cutlass::HGraphicsPipeline mLightingPipeline;

        std::optional<std::shared_ptr<CameraComponent>> mCamera;
        Cutlass::HBuffer mCameraUB;
        std::vector<std::shared_ptr<LightComponent>> mLights;
        Cutlass::HBuffer mLightUB;

        std::vector<RenderInfo> mRenderInfos;

        Cutlass::HCommandBuffer mShadowCB;
        Cutlass::HCommandBuffer mGeometryCB;
        Cutlass::HCommandBuffer mLightingCB;
        Cutlass::HCommandBuffer mForwardCB;
        Cutlass::HCommandBuffer mPostEffectCB;
        Cutlass::HCommandBuffer mSpriteCB;
        std::vector<Cutlass::HCommandBuffer> mPresentCBs;

        //サブコマンドバッファ
        std::vector<Cutlass::HCommandBuffer> mShadowSubs;
        std::vector<Cutlass::HCommandBuffer> mGeometrySubs;
        std::vector<Cutlass::HCommandBuffer> mLightingSubs;
        std::vector<Cutlass::HCommandBuffer> mForwardSubs;
        std::vector<Cutlass::HCommandBuffer> mPostEffectSubs;
        std::vector<Cutlass::HCommandBuffer> mSpriteSubs;

        bool mShadowAdded;
        bool mGeometryAdded;
        bool mLightingAdded;
        bool mForwardAdded;
        bool mPostEffectAdded;
        bool mSpriteAdded;

        Cutlass::HTexture mDebugTex;//!

        bool mSceneBuilded;
    };
};