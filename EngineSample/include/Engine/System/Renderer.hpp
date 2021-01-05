#pragma once

#include <Cutlass.hpp>

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

        Renderer(std::shared_ptr<Cutlass::Context> context, const std::vector<Cutlass::HWindow>& hwindows);

        //Noncopyable, Nonmoveable
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        virtual ~Renderer();

        virtual void regist(const std::shared_ptr<MeshComponent> const mesh, const std::shared_ptr<MaterialComponent> const material);

        virtual void addLight(std::shared_ptr<LightComponent> light);
        virtual void removeLight(std::shared_ptr<LightComponent> light);

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
        //描画対象・描画パス
        //テクスチャレンダリングパス
        std::vector<Cutlass::HTexture> mRTTexs;
        std::vector<Cutlass::HRenderPass> mTexPasses;
        //ウィンドウ描画パス 
        std::vector<Cutlass::HRenderPass> mPresentPasses;


        struct RenderInfo
        {
            std::shared_ptr<MeshComponent> mesh;
            std::shared_ptr<MaterialComponent> material;
            Cutlass::HBuffer vertexBuffer;
            Cutlass::HBuffer indexBuffer;
            
            std::vector<Cutlass::HBuffer> sceneCB;
            std::vector<Cutlass::HBuffer> materialCB;

            std::optional<Cutlass::HCommandBuffer> commandBuffer;
        };

        std::optional<std::shared_ptr<CameraComponent>> mCamera;
        std::vector<std::shared_ptr<LightComponent>> mLights;

        std::vector<RenderInfo> mRenderInfos;
        
    };
};