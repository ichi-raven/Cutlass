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
    Renderer::Renderer(std::shared_ptr<Context> context, const std::vector<HWindow>& hwindows, const uint16_t frameCount)
    : mContext(context)
    , mHWindows(hwindows)
    , mFrameCount(frameCount)
    , mSceneBuilded(false)
    {
        assert(Result::eSuccess == mContext->createTextureFromFile("../resources/textures/texture.png", mDebugTex));

        mRTTexs.reserve(hwindows.size());

        //描画用テクスチャ、テクスチャレンダリングパス, プレゼントパス
        for(const auto& hw : hwindows)
        {
            mPresentPasses.emplace_back();
            mContext->createRenderPass(hw, true, mPresentPasses.back().renderPass);

            {
                GraphicsPipelineInfo gpi
                (
                    Shader("../resources/shaders/present/vert.spv", "main"),
                    Shader("../resources/shaders/present/frag.spv", "main"),
                    mPresentPasses.back().renderPass,
                    DepthStencilState::eNone,
                    RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise, 1.f),
                    Topology::eTriangleStrip,
                    ColorBlend::eDefault,
                    MultiSampleState::eDefault
                );

                if (Result::eSuccess != mContext->createGraphicsPipeline(gpi, mPresentPasses.back().graphicsPipeline))
                    std::cout << "Failed to create present pipeline!\n";  
            }

            TextureInfo ti;
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
            ti.setRTTex2D(width, height);
            mContext->createTexture(ti, mRTTexs.emplace_back());
            ti.setRTTex2DDepth(width, height);
            mContext->createTexture(ti, mDepthBuffers.emplace_back());

            {//テクスチャの画面表示用コマンドを作成
                ShaderResourceSet SRSet;
                SRSet.setCombinedTexture(0, mRTTexs.back());
                
                for(auto& wp : mPresentPasses)
                {
                    std::vector<Cutlass::CommandList> cls;
                    cls.resize(mFrameCount);

                    Cutlass::ColorClearValue ccv{1.f, 0, 0, 0};
                    Cutlass::DepthClearValue dcv(1.f, 0);

                    for(size_t i = 0; i < cls.size(); ++i)
                    {
                        //for(const auto& target : mRTTexs)
                        cls[i].readBarrier(mRTTexs.back());
                        cls[i].beginRenderPass(wp.renderPass, true, ccv, dcv);
                        cls[i].bindGraphicsPipeline(wp.graphicsPipeline);
                        cls[i].bindShaderResourceSet(0, SRSet);
                        cls[i].render(4, 1, 0, 0);
                        cls[i].endRenderPass();
                        cls[i].present();
                    }

                    mPresentCBs.emplace_back();
                    mContext->createCommandBuffer(cls, mPresentCBs.back());
                }
            }
        }
        
        mTexPasses.emplace(RenderPassList::eTexClear, Cutlass::HRenderPass());
        mTexPasses.emplace(RenderPassList::eTex, Cutlass::HRenderPass());

        mContext->createRenderPass(RenderPassCreateInfo(mRTTexs, mDepthBuffers.back()), mTexPasses[RenderPassList::eTexClear]);
        mContext->createRenderPass(RenderPassCreateInfo(mRTTexs, mDepthBuffers.back(), true), mTexPasses[RenderPassList::eTex]);
    }

    Renderer::~Renderer()
    {
        //今の所手動で解放するものはない
    }

    void Renderer::regist(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<MaterialComponent>& material)
    {
        auto& tmp = mRenderInfos.emplace_back();
        tmp.mesh = mesh;
        tmp.material = material;

        Cutlass::HRenderPass rp;
        if(mRenderInfos.size() == 1)
            rp = mTexPasses[RenderPassList::eTexClear];
        else
            rp = mTexPasses[RenderPassList::eTex];
        
        GraphicsPipelineInfo gpi
        (
            material->getVS(),
            material->getFS(),
            rp,
            DepthStencilState::eDepth,
            mesh->getRasterizerState(),
            mesh->getTopology(),
            material->getColorBlend(),
            material->getMultiSampleState()
        );

        if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.pipeline))
            assert(!"failed to create graphics pipeline");

        {//コマンド作成
            ShaderResourceSet sceneSet;
            Cutlass::CommandList cl;
            Cutlass::HCommandBuffer cb;
            //ジオメトリ固有パラメータセット
            {
                Cutlass::HBuffer sceneCB;
                Cutlass::BufferInfo bi;
                bi.setUniformBuffer<SceneData>();
                mContext->createBuffer(bi, sceneCB);
                
                sceneSet.setUniformBuffer(0, sceneCB);
                sceneSet.setCombinedTexture(1, mDebugTex);
                tmp.sceneCB = sceneCB;
            }

            {//コマンドビルド
                
                if(mRenderInfos.size() == 1)
                    cl.beginRenderPass(rp, true);
                else
                    cl.beginRenderPass(rp, false);
                cl.bindVertexBuffer(mesh->getVB());
                cl.bindIndexBuffer(mesh->getIB());
                cl.bindGraphicsPipeline(tmp.pipeline);
                
                Cutlass::ShaderResourceSet materialSet;
                cl.bindShaderResourceSet(0, sceneSet);
                
                if(material->getMaterialSets().empty())
                {
                    cl.bindShaderResourceSet(1, materialSet);
                    cl.renderIndexed(mesh->getIndexNum(), 1, 0, 0, 0);        
                }
                else
                {
                    uint32_t renderedIndex = 0;
                    for(const auto& material : material->getMaterialSets())
                    {
                        materialSet.setUniformBuffer(0, material.paramBuffer);
                        if(material.texture)
                            materialSet.setCombinedTexture(1, material.texture.value());

                        cl.bindShaderResourceSet(1, materialSet);
                        if(material.useIndexNum)
                        {
                            cl.renderIndexed(material.useIndexNum.value(), 1, renderedIndex, 0, 0);
                            renderedIndex += material.useIndexNum.value();
                        }
                        else
                            cl.renderIndexed(mesh->getIndexNum(), 1, renderedIndex, 0, 0);
                    }
                }
                cl.endRenderPass();
            }

            if(Cutlass::Result::eSuccess != mContext->createCommandBuffer(cl, cb))
                assert(!"failed to create command buffer!");
            mCommandBuffers.emplace_back(cb);
        }
    }

    void Renderer::addLight(const std::shared_ptr<LightComponent>& light)
    {
        mLights.emplace_back(light);
    }

    void Renderer::setCamera(const std::shared_ptr<CameraComponent>& camera)
    {
        mCamera = camera;
    }

    void Renderer::clearScene()
    {
        mCamera = std::nullopt;
        mLights.clear();

        for(const auto& ri : mRenderInfos)
        {
            if(ri.sceneCB)
                mContext->destroyBuffer(ri.sceneCB.value());
            if(ri.lightCB)
                mContext->destroyBuffer(ri.lightCB.value());
            mContext->destroyGraphicsPipeline(ri.pipeline);
        }

        mRenderInfos.clear();

        for(const auto& cb : mCommandBuffers)
        {
            mContext->destroyGraphicsPipeline(cb);
        }

        mCommandBuffers.clear();
    }

    void Renderer::build()
    {
        if(!mCamera || !mCamera.value()->getEnable())
        {
           assert(!"no camera!");//カメラがないとなにも映りません
           return;
        }

        //現在の光源から光源データをビルド
        {
            //TODO
        }
        
        {//各定数バッファを書き込み
            SceneData sceneData;
            {//各ジオメトリ共通データ
                sceneData.view = mCamera.value()->getViewMatrix();
                sceneData.proj = mCamera.value()->getProjectionMatrix();
            }

            for(auto& geom : mRenderInfos)
            {
                //ジオメトリ固有パラメータセット
                sceneData.world = geom.mesh->getTransform().getWorldMatrix();
                mContext->writeBuffer(sizeof(SceneData), &sceneData, geom.sceneCB.value());
            }
        }
        
        mSceneBuilded = true;
    }

    void Renderer::render()
    {
        if(!mSceneBuilded)
        {
            build();
            mSceneBuilded = false;
        }

        for(const auto& cb : mCommandBuffers)
            mContext->execute(cb);

        for(const auto& cb : mPresentCBs)
            mContext->execute(cb);
    }
}