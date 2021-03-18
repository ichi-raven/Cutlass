#pragma once

#include <Cutlass.hpp>

#include <glm/glm.hpp>

#include <vector>

#include "../Actors/IActor.hpp"

namespace Engine
{
    class MeshComponent;
    class MaterialComponent;
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

        virtual void addLight(std::shared_ptr<LightComponent> light);

        //TODO
        //virtual void addPostEffect();

        //このカメラになる
        virtual void setCamera(std::shared_ptr<CameraComponent> camera);

        //現在設定されている情報から描画用シーンをビルドする
        virtual void buildScene();

        //描画コマンド実行
        virtual void render();

    protected:
        std::shared_ptr<Cutlass::Context> mContext;
        std::vector<Cutlass::HWindow> mHWindows;

    private:
        const uint16_t mFrameCount;

        //描画対象・描画パス
        //テクスチャレンダリングパス
        std::vector<Cutlass::HTexture> mRTTexs;        
        enum class RenderPassList
        {
            eTex,
        };
        std::unordered_map<RenderPassList, Cutlass::HRenderPass> mTexPasses;
        
        //ウィンドウ描画は複数パスが必要
        struct WindowPass
        {
            Cutlass::HRenderPass renderPass;
            Cutlass::HGraphicsPipeline graphicsPipeline;
        };
        std::vector<WindowPass> mPresentPasses;
        std::vector<Cutlass::HCommandBuffer> mPresentCBs;

        struct SceneData
        {
            glm::mat4 world;
            glm::mat4 view;
            glm::mat4 proj;
        };

        struct RenderInfo
        {
            std::shared_ptr<MeshComponent> mesh;
            std::shared_ptr<MaterialComponent> material;
            
            std::optional<Cutlass::HBuffer> sceneCB;
            std::optional<Cutlass::HBuffer> lightCB;
            
            Cutlass::HGraphicsPipeline pipeline;
        };

        std::optional<std::shared_ptr<CameraComponent>> mCamera;
        std::vector<std::shared_ptr<LightComponent>> mLights;

        std::vector<RenderInfo> mRenderInfos;

        std::vector<Cutlass::HCommandBuffer> mCommandBuffers;

        Cutlass::HTexture mDebugTex;
    };
};