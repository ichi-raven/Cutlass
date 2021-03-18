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
    {
        assert(Result::eSuccess == mContext->createTextureFromFile("../resources/textures/texture.png", mDebugTex));

        mTexPasses.emplace(RenderPassList::eTex, Cutlass::HRenderPass());

        //描画用テクスチャ、テクスチャレンダリングパス, プレゼントパス
        for(const auto& hw : hwindows)
        {
            
            mPresentPasses.emplace_back();
            mContext->createRenderPass(hw, true, mPresentPasses.back().renderPass);
            {
                ShaderResourceDesc SRDesc;
                SRDesc.layout.allocForCombinedTexture(0);
                SRDesc.layout.allocForUniformBuffer(1);
                SRDesc.setCount = frameCount;

                GraphicsPipelineInfo rpi
                (
                    ColorBlend::eDefault,
                    Topology::eTriangleStrip,
                    RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise, 1.f),
                    MultiSampleState::eDefault,
                    DepthStencilState::eNone,
                    Shader("../resources/shaders/present/vert.spv", "main"),
                    Shader("../resources/shaders/present/frag.spv", "main"),
                    SRDesc,
                    mPresentPasses.back().renderPass
                );

                if (Result::eSuccess != mContext->createGraphicsPipeline(rpi, mPresentPasses.back().graphicsPipeline))
                    std::cout << "Failed to create present pipeline!\n";  
            }

            TextureInfo ti;
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
            ti.setRTTex2D(width, height);
            mContext->createTexture(ti, mRTTexs.emplace_back());

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
                        for(auto& target : mRTTexs)
                            cls[i].readBarrier(target);
                        cls[i].beginRenderPass(wp.renderPass, true, ccv, dcv);
                        cls[i].bindGraphicsPipeline(wp.graphicsPipeline);
                        cls[i].bindShaderResourceSet(SRSet);
                        cls[i].render(4, 1, 0, 0);
                        cls[i].endRenderPass();
                        cls[i].present();
                    }

                    mPresentCBs.emplace_back();
                    mContext->createCommandBuffer(cls, mPresentCBs.back());
                }
            }
        }
        
        mContext->createRenderPass(RenderPassCreateInfo(mRTTexs), mTexPasses[RenderPassList::eTex]);
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
        
        ShaderResourceDesc SRDesc;
        {//シェーダリソース設計
            SRDesc.layout.allocForUniformBuffer(0);//Scene
            SRDesc.layout.allocForCombinedTexture(1);//Material Texture
            SRDesc.layout.allocForUniformBuffer(2);//Material
            SRDesc.layout.allocForUniformBuffer(3);//Light
            SRDesc.setCount = std::max(4, static_cast<int>(4 * material->getMaterialSets().size()));
        }

        GraphicsPipelineInfo gpi
        (
            mesh->getVertexLayout(),
            material->getColorBlend(),
            material->getTopology(),
            material->getRasterizerState(),
            material->getMultiSampleState(),
            DepthStencilState::eDepth,
            material->getVS(),
            material->getFS(),
            SRDesc,
            mTexPasses[RenderPassList::eTex]
        );

        if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.pipeline))
            assert(!"failed to create graphics pipeline");

        {//コマンド作成
            ShaderResourceSet SRSet;
            Cutlass::CommandList cl;
            Cutlass::HCommandBuffer cb;
            //ジオメトリ固有パラメータセット
            {
                Cutlass::BufferInfo bi;
                SceneData sceneData;
                Cutlass::HBuffer sceneCB;
                sceneData.world = mesh->getTransform().getWorldMatrix();
                bi.setUniformBuffer<SceneData>();
                mContext->createBuffer(bi, sceneCB);
                SRSet.setUniformBuffer(0, sceneCB);
                SRSet.setCombinedTexture(1, mDebugTex);
                tmp.sceneCB = sceneCB;
            }

            {//コマンドビルド
                cl.beginRenderPass(mTexPasses[RenderPassList::eTex]);
                cl.bindVertexBuffer(mesh->getVB());
                cl.bindIndexBuffer(mesh->getIB());
                cl.bindGraphicsPipeline(tmp.pipeline);
                if(material->getMaterialSets().empty())
                {
                    cl.bindShaderResourceSet(SRSet);
                    cl.renderIndexed(mesh->getIndexNum(), 1, 0, 0, 0);        
                }
                else
                {
                    uint32_t renderedIndex = 0;
                    for(const auto& material : material->getMaterialSets())
                    {
                        SRSet.setUniformBuffer(1, material.paramBuffer);
                        if(material.texture)
                            SRSet.setCombinedTexture(2, material.texture.value());

                        cl.bindShaderResourceSet(SRSet);
                        if(material.useVertexNum)
                        {
                            cl.renderIndexed(material.useVertexNum.value(), 1, renderedIndex, 0, 0);
                            renderedIndex += material.useVertexNum.value();
                        }
                        else
                            cl.renderIndexed(mesh->getIndexNum(), 1, renderedIndex, 0, 0);
                    }
                }
                cl.endRenderPass();
            }

            mContext->createCommandBuffer(cl, cb);
            mCommandBuffers.emplace_back(cb);
        }
    }

    void Renderer::addLight(std::shared_ptr<LightComponent> light)
    {
        mLights.emplace_back(light);
    }

    void Renderer::setCamera(std::shared_ptr<CameraComponent> camera)
    {
        mCamera = camera;
    }

    void Renderer::buildScene()
    {
        if(!mCamera || !mCamera.value()->getEnable())
        {
           assert(!"camera is nothing!");//カメラがないとなにも映りません
           return;
        }

        //現在の光源から光源データをビルド
        {
            //TODO
        }
        
        {//各定数バッファをビルドし、コマンドを作成
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

                // std::cout << "world : \n";
                // for(int i = 0; i < 4; ++i)
                // {
                //     for(int j = 0; j < 4; ++j)
                //         std::cout << sceneData.world[i][j] << " ";
                //     std::cout << "\n";
                // }

                // std::cout << "view : \n";
                // for(int i = 0; i < 4; ++i)
                // {
                //     for(int j = 0; j < 4; ++j)
                //         std::cout << sceneData.view[i][j] << " ";
                //     std::cout << "\n";
                // }

                // std::cout << "proj : \n";
                // for(int i = 0; i < 4; ++i)
                // {
                //     for(int j = 0; j < 4; ++j)
                //         std::cout << sceneData.proj[i][j] << " ";
                //     std::cout << "\n";
                // }
            }
        }
    }

    void Renderer::render()
    {
        for(const auto& cb : mCommandBuffers)
            mContext->execute(cb);

        for(const auto& cb : mPresentCBs)
            mContext->execute(cb);
    }

}