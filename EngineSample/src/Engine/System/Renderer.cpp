#include <Engine/System/Renderer.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/CameraComponent.hpp>
#include <Engine/Components/LightComponent.hpp>

#include <iostream>

#include <glm/gtc/quaternion.hpp>

using namespace Cutlass;

namespace Engine
{
    Renderer::Renderer(std::shared_ptr<Context> context, const std::vector<HWindow>& hwindows)
    : mContext(context)
    , mHWindows(hwindows)
    {

        //描画用テクスチャ、テクスチャレンダリングパス, プレゼントパス
        for(const auto& hw : hwindows)
        {
            context->createRenderPass(hw, mPresentPasses.emplace_back());

            TextureInfo ti;
            uint32_t width, uint32_t height;
            mContext->getWindowSize(hw, width, height);
            ti.setRTTex2D(width, height);
            mContext->createTexture(ti, mRTTexs.emplace_back());

            mContext->createRenderPass(RenderPassCreateInfo(mRTTexs.back()))
        }
    }

    Renderer::~Renderer()
    {

    }

    void Renderer::regist(const std::shared_ptr<MeshComponent> const mesh, const std::shared_ptr<MaterialComponent> const material)
    {
        auto&& tmp = mRenderInfos.emplace_back();
        tmp.mesh = mesh;
        tmp.material = material;
        //迷っている
        tmp.vertexBuffer = mesh->getVB();
        tmp.indexBuffer = mesh->getIB();
    }

    // void Renderer::init(std::shared_ptr<Context> context, const uint32_t width, const uint32_t height, const uint32_t frameCount, const std::vector<Cutlass::HWindow>& hwindows)
    // {
    //     // mWidth = width;
    //     // mHeight = height;
    //     // mFrameCount = frameCount;
        
    //     // context.createRenderPass(RenderPassCreateInfo(window, true), mWindowPass);
    //     // {
    //     //     TextureInfo ti;
    //     //     ti.setRTTex2D(1920, 1080);
    //     //     if(Result::eSuccess != context.createTexture(ti, mRTTex))
    //     //         return;
    //     // }

    //     // if(Result::eSuccess != context.createRenderPass(RenderPassCreateInfo(mRTTex), mIntermediatePass))
    //     //     return;
        
    //     // //定数バッファ類を確保
    //     // mMVPCBs.resize(mFrameCount);
    //     // mSceneCBs.resize(mFrameCount);

    //     // //いろんな描画パスを定義しておく
    //     // //ただし、SetCountだけ現時点で確定できない
    //     // //基本描画パス
    //     // // {
    //     // //     mR2TPassInfo.renderPass = mIntermediatePass;
    //     // //     mR2TPassInfo.SRDesc.layout.allocForUniformBuffer(0);
    //     // //     mR2TPassInfo.SRDesc.layout.allocForUniformBuffer(1);
    //     // //     mR2TPassInfo.SRDesc.layout.allocForCombinedTexture(2);
    //     // //     mR2TPassInfo.SRDesc.layout.allocForUniformBuffer(3);
    //     // //     mR2TPassInfo.vertexLayout = std::make_optional(MeshComponent::getVertexLayout());
    //     // //     mR2TPassInfo.multiSampleState = MultiSampleState::eDefault;
    //     // //     mR2TPassInfo.topology = Topology::eTriangleList;
    //     // //     mR2TPassInfo.VS = Shader("../Resources/Shaders/MeshWithMaterial/vert.spv", "main");
    //     // //     mR2TPassInfo.FS = Shader("../Resources/Shaders/MeshWithMaterial/frag.spv", "main");
    //     // // }

    //     // // {//プレゼント・パス(プレゼント・タイム)(笑い声)
    //     // //     mPresentPassInfo.renderDST = windowRDST;
    //     // //     mPresentPassInfo.SRDesc.layout.allocForUniformBuffer(0);
    //     // //     mPresentPassInfo.SRDesc.layout.allocForCombinedTexture(1);
    //     // //     mPresentPassInfo.multiSampleState = MultiSampleState::eDefault;
    //     // //     mPresentPassInfo.topology = Topology::eTriangleList;
    //     // //     mPresentPassInfo.VS = Shader("../Resources/Shaders/presentTexture/vert.spv", "main");
    //     // //     mPresentPassInfo.VS = Shader("../Resources/Shaders/presentTexture/frag.spv", "main");
    //     // // }
    // }
    
    // void Renderer::addMesh(const std::shared_ptr<MeshComponent> mesh, const std::shared_ptr<MaterialComponent> material)
    // {
    //     if(!mesh->getEnable())
    //         return;
    //     mMeshWithMaterialQueue.emplace(mesh, material);
    // }

    // void Renderer::addCamera(const std::shared_ptr<CameraComponent> camera)
    // {
    //     // if(!camera->getEnable() || mCamera == camera)
    //     //     return;
    //     // mCamera = camera;
    // }

    // void Renderer::addLight(const std::shared_ptr<LightComponent> light)
    // {
    //     // if(!light->getEnable())
    //     //     return;
    //     // mLightQueue.emplace(light);
    // }

    // void Renderer::render(Context& context)
    // {
    //     //毎フレームRenderPipeline実体, CommandBuffer実体を構築して描画する
    //     //Cameraが無けりゃ描画はできません
    //     std::cerr << "updated!\n";
    // }

}