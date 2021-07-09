#include <Engine/System/Renderer.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/CustomMaterialComponent.hpp>
#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/SkeltalMeshComponent.hpp>
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
        uint32_t maxWidth = 0, maxHeight = 0;
        for(const auto& hw : hwindows)
        {
            HRenderPass rp;
            mContext->createRenderPass(RenderPassInfo(hw), rp);
            GraphicsPipelineInfo gpi
            (
                Shader("../resources/shaders/present/vert.spv", "main"),
                Shader("../resources/shaders/present/frag.spv", "main"),
                rp,
                DepthStencilState::eNone,
                RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise, 1.f),
                Topology::eTriangleStrip,
                ColorBlend::eDefault,
                MultiSampleState::eDefault
            );

            HGraphicsPipeline gp;
            if (Result::eSuccess != mContext->createGraphicsPipeline(gpi, gp))
                assert(!"Failed to create present pipeline!\n");  
            
            mPresentPasses.emplace_back(rp, gp);

            TextureInfo ti;
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
            if(maxWidth < width)
                maxWidth = width;
            if(maxHeight < height)
                maxHeight = height;
                
            ti.setRTTex2D(width, height);
            mContext->createTexture(ti, mRTTexs.emplace_back());
            ti.setRTTex2DDepth(width, height);
            mContext->createTexture(ti, mDepthBuffers.emplace_back());

            {//テクスチャの画面表示用コマンドを作成
                ShaderResourceSet SRSet[mFrameCount];
                for(size_t i = 0; i < mFrameCount; ++i)
                    SRSet[i].bind(0, mRTTexs.back());
                
                for(auto& wp : mPresentPasses)
                {
                    std::vector<Cutlass::CommandList> cls;
                    cls.resize(mFrameCount);

                    Cutlass::ColorClearValue ccv{1.f, 0, 0, 0};
                    Cutlass::DepthClearValue dcv(1.f, 0);

                    for(size_t i = 0; i < cls.size(); ++i)
                    {
                        //for(const auto& target : mRTTexs)
                        cls[i].barrier(mRTTexs.back());
                        cls[i].begin(wp.first, true, ccv, dcv);
                        cls[i].bind(wp.second);
                        cls[i].bind(0, SRSet[i]);
                        cls[i].render(4, 1, 0, 0);
                        cls[i].end();
                    }

                    mPresents.emplace_back();
                    mContext->createCommandBuffer(cls, mPresents.back());
                }
            }
        }

        //デフォルトシェーダ
        mDefferedVS = Shader("../resources/shaders/HLSLDeffered/GBuffer_vert.spv", "VSMain");
        mDefferedFS = Shader("../resources/shaders/HLSLDeffered/GBuffer_frag.spv", "PSMain");
        mDefferedSkinVS = Shader("../resources/shaders/HLSLDefferedGLTFSkin/GBuffer_vert.spv", "VSMain");
        mDefferedSkinFS = Shader("../resources/shaders/HLSLDefferedGLTFSkin/GBuffer_frag.spv", "PSMain");
        mLightingVS = Shader("../resources/shaders/HLSLDeffered/Lighting_vert.spv", "VSMain");
        mLightingFS = Shader("../resources/shaders/HLSLDeffered/Lighting_frag.spv", "PSMain");

        {//G-Buffer 構築
            Cutlass::TextureInfo ti;
            ti.setRTTex2DColor(maxWidth, maxHeight);
            mContext->createTexture(ti, mGBuffer.albedoRT);
            ti.format = Cutlass::ResourceType::eF32Vec4;
            mContext->createTexture(ti, mGBuffer.normalRT);
            mContext->createTexture(ti, mGBuffer.worldPosRT);
            ti.setRTTex2DDepth(maxWidth, maxHeight);
            mContext->createTexture(ti, mGBuffer.depthBuffer);

            mContext->createRenderPass(RenderPassInfo({mGBuffer.albedoRT, mGBuffer.worldPosRT, mGBuffer.normalRT}, mGBuffer.depthBuffer, true), mGBuffer.renderPass);
        }

        {//ライティング用パス
            if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo(mRTTexs.back(), true), mLightingPass))
                assert(!"failed to create lighting renderpass!");

            GraphicsPipelineInfo gpi
            (
                mLightingVS,
                mLightingFS,
                mLightingPass,
                DepthStencilState::eNone,
                RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
                Topology::eTriangleStrip
            );

            mContext->createGraphicsPipeline(gpi, mLightingPipeline);
        }

    }

    Renderer::~Renderer()
    {
        
    }

    void Renderer::regist(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<MaterialComponent>& material)
    {
        auto& tmp = mRenderInfos.emplace_back();
        tmp.mesh = mesh;
        tmp.material = material;

        GraphicsPipelineInfo gpi
        (
            mDefferedVS,
            mDefferedFS,
            mGBuffer.renderPass,
            DepthStencilState::eDepth,
            mesh->getRasterizerState(),
            mesh->getTopology(),
            ColorBlend::eDefault,
            MultiSampleState::eDefault
        );
 
        if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.pipeline))
            assert(!"failed to create graphics pipeline");

        {//コマンド作成
            //ジオメトリ固有パラメータセット
            {
                Cutlass::HBuffer sceneCB;
                Cutlass::BufferInfo bi;
                bi.setUniformBuffer<SceneData>();
                mContext->createBuffer(bi, sceneCB);
                tmp.sceneCB = sceneCB;
            }

            ShaderResourceSet bufferSet;
            ShaderResourceSet textureSet;
            bufferSet.bind(0, tmp.sceneCB.value());

            bufferSet.bind(1, material->getMaterialSets().back().paramBuffer);
            textureSet.bind(0, mDebugTex);

            CommandList cl;
            cl.begin(mGBuffer.renderPass);
            cl.bind(tmp.pipeline);
            cl.bind(mesh->getVB(), mesh->getIB());
            cl.bind(0, bufferSet);
            cl.bind(1, textureSet);
            cl.renderIndexed(mesh->getIndexNum(), 1, 0, 0, 0);
            cl.end();

            HCommandBuffer cb;
            if(Cutlass::Result::eSuccess != mContext->createCommandBuffer(cl, cb))
                assert(!"failed to create command buffer!");
            mGeometries.emplace_back(cb);
        }
    }

    void Renderer::regist(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<CustomMaterialComponent>& material)
    {

    }

    void Renderer::regist(const std::shared_ptr<SkeltalMeshComponent>& mesh, const std::shared_ptr<CustomMaterialComponent>& material)
    {

    }

    void Renderer::addLight(const std::shared_ptr<LightComponent>& light)
    {
        mLights.emplace_back(light->getLightCB().value());

        switch(light->getType())
        {
            case LightComponent::LightType::eDirectionalLight:

            break;
            case LightComponent::LightType::ePointLight:
                assert(!"TODO");
            break;
            default:
                assert(!"invalid light type!");
            break;
        }

        ShaderResourceSet bufferSet, textureSet;
        {//テクスチャレンダリングパスのリソースセット
            bufferSet.bind(0, mLights.back());
            //bufferSet.bind(1, sceneUB);

            textureSet.bind(0, mGBuffer.albedoRT);
            textureSet.bind(1, mGBuffer.normalRT);
            textureSet.bind(2, mGBuffer.worldPosRT);
        }

        CommandList cl;
        cl.barrier(mGBuffer.albedoRT);
        cl.barrier(mGBuffer.normalRT);
        cl.barrier(mGBuffer.worldPosRT);

        cl.begin(mLightingPass);
        cl.bind(mLightingPipeline);
        cl.bind(0, bufferSet);
        cl.bind(1, textureSet);
        cl.render(4, 1, 0, 0);
        cl.end();

        HCommandBuffer cb;
        mContext->createCommandBuffer(cl, cb);

        mLightings.emplace_back(cb);

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
            mContext->destroyGraphicsPipeline(ri.pipeline);
        }

        mRenderInfos.clear();

        for(const auto& cb : mGeometries)
        {
            mContext->destroyCommandBuffer(cb);
        }

        for(const auto& cb : mLightings)
        {
            mContext->destroyCommandBuffer(cb);
        }

        for(const auto& cb : mForwards)
        {
            mContext->destroyCommandBuffer(cb);
        }

        mGeometries.clear();
        mLights.clear();
        mForwards.clear();
    }

    void Renderer::build()
    {
        if(!mCamera || !mCamera.value()->getEnable())
        {
           assert(!"no camera!");//カメラがないとなにも映りません
           return;
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

        for(const auto& cb : mGeometries)
            mContext->execute(cb);

        for(const auto& cb : mLightings)
            mContext->execute(cb);

        for(const auto& cb : mForwards)
            mContext->execute(cb);

        for(const auto& cb : mPresents)
            mContext->execute(cb);
    }
}