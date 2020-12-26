#include <Engine/System/BaseRenderer.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/CameraComponent.hpp>
#include <Engine/Components/LightComponent.hpp>

#include <iostream>

#include <glm/gtc/quaternion.hpp>

namespace Engine
{
    
    void BaseRenderer::init(Cutlass::Context& context, const uint32_t width, const uint32_t height, const uint32_t frameCount, const Cutlass::HRenderDST& windowRDST)
    {
        mWidth = width;
        mHeight = height;
        mFrameCount = frameCount;
        mWindowRDST = windowRDST;

        {
            Cutlass::TextureInfo ti;
            ti.setRTTex2D(1920, 1080);
            if(Cutlass::Result::eSuccess != context.createTexture(ti, mRTTex))
                return;
        }

        if(Cutlass::Result::eSuccess != context.createRenderDST(mRTTex, mIntermediateDST))
            return;
        
        //定数バッファ類を確保
        mMVPCBs.resize(mFrameCount);
        mSceneCBs.resize(mFrameCount);

        //いろんな描画パスを定義しておく
        //ただし、SetCountだけ現時点で確定できない
        {//基本描画パス
            mR2TPassInfo.renderDST = mIntermediateDST;
            mR2TPassInfo.SRDesc.layout.allocForUniformBuffer(0);
            mR2TPassInfo.SRDesc.layout.allocForUniformBuffer(1);
            mR2TPassInfo.SRDesc.layout.allocForCombinedTexture(2);
            mR2TPassInfo.SRDesc.layout.allocForUniformBuffer(3);
            mR2TPassInfo.vertexLayout = std::make_optional(MeshComponent::getVertexLayout());
            mR2TPassInfo.multiSampleState = Cutlass::MultiSampleState::eDefault;
            mR2TPassInfo.topology = Cutlass::Topology::eTriangleList;
            mR2TPassInfo.VS = Cutlass::Shader("../Resources/Shaders/MeshWithMaterial/vert.spv", "main");
            mR2TPassInfo.FS = Cutlass::Shader("../Resources/Shaders/MeshWithMaterial/frag.spv", "main");
        }

        {//プレゼント・パス(プレゼント・タイム)(笑い声)
            mPresentPassInfo.renderDST = windowRDST;
            mPresentPassInfo.SRDesc.layout.allocForUniformBuffer(0);
            mPresentPassInfo.SRDesc.layout.allocForCombinedTexture(1);
            mPresentPassInfo.multiSampleState = Cutlass::MultiSampleState::eDefault;
            mPresentPassInfo.topology = Cutlass::Topology::eTriangleList;
            mPresentPassInfo.VS = Cutlass::Shader("../Resources/Shaders/presentTexture/vert.spv", "main");
            mPresentPassInfo.VS = Cutlass::Shader("../Resources/Shaders/presentTexture/frag.spv", "main");
        }
    }
    
    void BaseRenderer::addMesh(const std::shared_ptr<MeshComponent> mesh, const std::shared_ptr<MaterialComponent> material)
    {
        if(!mesh->getEnable())
            return;
        mMeshWithMaterialQueue.emplace(mesh, material);
    }

    void BaseRenderer::addCamera(const std::shared_ptr<CameraComponent> camera)
    {
        if(!camera->getEnable() || mCamera == camera)
            return;
        mCamera = camera;
    }

    void BaseRenderer::addLight(const std::shared_ptr<LightComponent> light)
    {
        if(!light->getEnable())
            return;
        mLightQueue.emplace(light);
    }

    void BaseRenderer::render(Cutlass::Context& context)
    {
        //毎フレームRenderPipeline実体, CommandBuffer実体を構築して描画する
        //Cameraが無けりゃ描画はできません
        assert(mCamera);
        
        const uint32_t currentFrame = context.getFrameBufferIndex(mWindowRDST);

        Cutlass::HRenderPipeline r2trp;
        mR2TPassInfo.SRDesc.setCount = mMeshWithMaterialQueue.size();
        context.createRenderPipeline(mR2TPassInfo, r2trp);
        Cutlass::CommandList commandList;

        //書き込み
        commandList.beginRenderPipeline(r2trp);
        while(!mMeshWithMaterialQueue.empty())
        {
            auto&& mesh = mMeshWithMaterialQueue.back().first;
            auto&& material = mMeshWithMaterialQueue.back().second;

            {//MVPバッファ書き込み
                MVP mvp;
                mvp.world = mesh->getTransform().getWorldMatrix();
                mvp.view = mCamera->getViewMatrix();
                mvp.proj = glm::perspective(glm::radians(45.f), 1.f * mWidth / mHeight, 1.f, 100.f);
                mvp.proj[1][1] *= -1;
                context.writeBuffer(sizeof(MVP), &mvp, mMVPCBs[(currentFrame + (mFrameCount - 1)) % mFrameCount]);
            }

            {//Sceneバッファ書き込み
                //まだ実装していません
            }

            Cutlass::ShaderResourceSet SRSet;
            {//SRSet構築
                SRSet.setUniformBuffer(0, );
            }

            commandList.bindVB(mesh->getVB());
            commandList.bindIB(mesh->getIB());
            commandList.bindSRSet()

            mMeshWithMaterialQueue.pop();
        }
        commandList.endRenderPipeline();
        

        context.execute()

        mCamera.reset();
    }
}