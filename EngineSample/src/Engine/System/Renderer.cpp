#include <Engine/System/Renderer.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/CustomMaterialComponent.hpp>
#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/SkeltalMeshComponent.hpp>
#include <Engine/Components/CameraComponent.hpp>
#include <Engine/Components/LightComponent.hpp>

#include <iostream>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace Cutlass;

namespace Engine
{
    Renderer::Renderer(std::shared_ptr<Context> context, const std::vector<HWindow>& hwindows, const uint16_t frameCount)
    : mContext(context)
    , mHWindows(hwindows)
    , mFrameCount(frameCount)
    , mSceneBuilded(false)
    , mShadowAdded(false)
    , mGeometryAdded(true)
    , mLightingAdded(false)
    , mForwardAdded(false)
    , mPostEffectAdded(false)
    , mSpriteAdded(false)
    {
        assert(Result::eSuccess == mContext->createTextureFromFile("../resources/textures/texture.png", mDebugTex));

        mRTTexs.reserve(hwindows.size());
        uint32_t maxWidth = 0, maxHeight = 0;

        for(const auto& hw : hwindows)
        {
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
            if(maxWidth < width)
                maxWidth = width;
            if(maxHeight < height)
                maxHeight = height;
        }

        {//G-Buffer 構築
            Cutlass::TextureInfo ti;
            constexpr float coef = 1.5;
            ti.setRTTex2DColor(maxWidth * coef, maxHeight * coef);
            mContext->createTexture(ti, mGBuffer.albedoRT);
            ti.format = Cutlass::ResourceType::eF32Vec4;
            mContext->createTexture(ti, mGBuffer.normalRT);
            mContext->createTexture(ti, mGBuffer.worldPosRT);
            ti.setRTTex2DDepth(maxWidth * coef, maxHeight * coef);
            mContext->createTexture(ti, mGBuffer.depthBuffer);

            if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo({mGBuffer.albedoRT, mGBuffer.normalRT, mGBuffer.worldPosRT}, mGBuffer.depthBuffer, false), mGBuffer.renderPass))
            {
                assert(!"failed to create G-Buffer renderpass!");
            }
        }

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
                
            ti.setRTTex2DColor(width, height);
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

                    mPresentCBs.emplace_back();
                    mContext->createCommandBuffer(cls, mPresentCBs.back());
                }
            }
        }


        //デフォルトシェーダ
        mShadowVS           = Shader("../resources/shaders/HLSLShadow/shadow_vert.spv", "VSMain");
        mShadowPS           = Shader("../resources/shaders/HLSLShadow/shadow_frag.spv", "PSMain");
        mDefferedVS         = Shader("../resources/shaders/HLSLDeffered/GBuffer_vert.spv", "VSMain");
        mDefferedFS         = Shader("../resources/shaders/HLSLDeffered/GBuffer_frag.spv", "PSMain");
        mDefferedSkinVS     = Shader("../resources/shaders/HLSLDefferedGLTFSkin/GBuffer_vert.spv", "VSMain");
        mDefferedSkinFS     = Shader("../resources/shaders/HLSLDefferedGLTFSkin/GBuffer_frag.spv", "PSMain");
        mLightingVS         = Shader("../resources/shaders/HLSLDeffered/Lighting_vert.spv", "VSMain");
        mLightingFS         = Shader("../resources/shaders/HLSLDeffered/Lighting_frag.spv", "PSMain");

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
                Topology::eTriangleStrip,
                ColorBlend::eAlphaBlend
            );

            mContext->createGraphicsPipeline(gpi, mLightingPipeline);
        }

        //デフォルトコマンド構築
        CommandList cl;
        cl.clear();
        mContext->createCommandBuffer(cl, mShadowCB);
        mContext->createCommandBuffer(cl, mGeometryCB);
        mContext->createCommandBuffer(cl, mLightingCB);
        mContext->createCommandBuffer(cl, mForwardCB);
        mContext->createCommandBuffer(cl, mPostEffectCB);
        mContext->createCommandBuffer(cl, mSpriteCB);
        
        {//カメラ用バッファを作成しておく
            BufferInfo bi;
            bi.setUniformBuffer<CameraData>();
            if(Result::eSuccess != mContext->createBuffer(bi, mCameraUB))
                assert(!"failed to create camera UB!");
        }
        
        {//ライト用バッファも作成しておく
            BufferInfo bi;
            bi.setUniformBuffer<LightData>();
            if(Result::eSuccess != mContext->createBuffer(bi, mLightUB))
                assert(!"failed to create light UB!");
        }

        // {//ライティング用コマンドバッファ
        //     ShaderResourceSet bufferSet, textureSet;
        //     {
        //         bufferSet.bind(0, mLightUB);
        //         bufferSet.bind(1, mCameraUB);

        //         textureSet.bind(0, mGBuffer.albedoRT);
        //         textureSet.bind(0, mGBuffer.normalRT);
        //         textureSet.bind(0, mGBuffer.worldPosRT);
        //     }

        //     CommandList cl;
        //     cl.begin(mLightingPass);
        //     cl.bind(mLightingPipeline);
        //     cl.bind(0, bufferSet);
        //     cl.bind(1, textureSet);
        //     cl.render(4, 1, 0, 0);
        //     cl.end();
        //     mContext->updateCommandBuffer(cl, mLightingCB);
        // }
    }

    Renderer::~Renderer()
    {
        
    }

    void Renderer::regist(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<MaterialComponent>& material)
    {
        std::cerr << "regist start\n";
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
            mesh->getTopology()
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
            {
                bufferSet.bind(0, tmp.sceneCB.value());
                if(!material->getMaterialSets().empty())
                    bufferSet.bind(1, material->getMaterialSets().back().paramBuffer);
                else
                {
                    //マテリアル読む
                }
                textureSet.bind(0, mDebugTex);
            }

            //ジオメトリパス
            SubCommandList scl(mGBuffer.renderPass);
            scl.bind(tmp.pipeline);
            scl.bind(mesh->getVB(), mesh->getIB());
            scl.bind(0, bufferSet);
            scl.bind(1, textureSet);
            scl.renderIndexed(mesh->getIndexNum(), 1, 0, 0, 0);

            HCommandBuffer cb;
            if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, cb))
                assert(!"failed to create command buffer!");
            mGeometrySubs.emplace_back(cb);
        }

        mGeometryAdded = true;
        std::cerr << "registed\n";
    }

    void Renderer::regist(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<CustomMaterialComponent>& material)
    {
        mForwardAdded = true;
    }

    void Renderer::regist(const std::shared_ptr<SkeltalMeshComponent>& mesh, const std::shared_ptr<CustomMaterialComponent>& material)
    {

    }

    void Renderer::addLight(const std::shared_ptr<LightComponent>& light)
    {
        //std::cerr << "light start\n";
        mLights.emplace_back(light);
        if(!light->getLightUB())
        {
            assert(!"not builded light!");
        }

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
        {
            bufferSet.bind(0, light->getLightUB().value());
            bufferSet.bind(1, mCameraUB);

            textureSet.bind(0, mGBuffer.albedoRT);
            textureSet.bind(1, mGBuffer.normalRT);
            textureSet.bind(2, mGBuffer.worldPosRT);
        }

        SubCommandList scl(mLightingPass);
        scl.bind(mLightingPipeline);
        scl.bind(0, bufferSet);
        scl.bind(1, textureSet);
        scl.render(4, 1, 0, 0);

        HCommandBuffer cb;
        mContext->createSubCommandBuffer(scl, cb);

        // HCommandBuffer cb;
        // CommandList cl;
        // cl.begin(mLightingPass);
        // cl.bind(mLightingPipeline);
        // cl.bind(0, bufferSet);
        // cl.bind(1, textureSet);
        // cl.render(4, 1, 0, 0);
        // cl.end();
        //mContext->createCommandBuffer(cl, cb);

        mLightingSubs.emplace_back(cb);
        mLightingAdded = true;
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

        for(const auto& cb : mShadowSubs)
        {
            mContext->destroyCommandBuffer(cb);
        }

        for(const auto& cb : mGeometrySubs)
        {
            mContext->destroyCommandBuffer(cb);
        }

        // for(const auto& cb : mLightingSubs)
        // {
        //     mContext->destroyCommandBuffer(cb);
        // }

        for(const auto& cb : mForwardSubs)
        {
            mContext->destroyCommandBuffer(cb);
        }

        for(const auto& cb : mPostEffectSubs)
        {
            mContext->destroyCommandBuffer(cb);
        }

        for(const auto& cb : mSpriteSubs)
        {
            mContext->destroyCommandBuffer(cb);
        }

        mShadowSubs.clear();
        mGeometrySubs.clear();
        //mLightingSubs.clear();
        mForwardSubs.clear();
        mPostEffectSubs.clear();
        mSpriteSubs.clear();
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

            for(auto& ri : mRenderInfos)
            {
                //ジオメトリ固有パラメータセット
                sceneData.world = ri.mesh->getTransform().getWorldMatrix();
                mContext->writeBuffer(sizeof(SceneData), &sceneData, ri.sceneCB.value());
            }

            {//カメラ
                CameraData data;
                data.cameraPos = mCamera.value()->getTransform().getPos();
                //std::cerr << "camera pos : " << glm::to_string(data.cameraPos) << "\n";
                mContext->writeBuffer(sizeof(CameraData), &data, mCameraUB);
            }

            // {//ライト
            //     LightData data;
            //     //auto limit = std::min(static_cast<size_t>(MAXLIGHTNUM), mLights.size());
            //     //data.lightNum = limit;
            //     LightComponent::DirectionalLightParam dparam;
            //     dparam = std::get<LightComponent::DirectionalLightParam>(mLights[0]->getParam());
            //     data.lightDir = dparam.lightDir;
            //     data.lightColor = dparam.lightColor;
            //     // for(size_t i = 0; i < MAXLIGHTNUM; ++i)
            //     // {
            //     //     if(i >= mLights.size())
            //     //     {
            //     //         //data.lightDir[i] = glm::vec4(0, 1.f, 1.f, 0);
            //     //         //data.lightColor[i] = glm::vec4(0.3f, 0.3f, 0.3f, 1.f);
            //     //         continue;
            //     //     }
            //     //     switch(mLights[i]->getType())
            //     //     {
            //     //         case LightComponent::LightType::eDirectionalLight:
            //     //         break;
            //     //         case LightComponent::LightType::ePointLight:
            //     //             assert(!"TODO");
            //     //         break;
            //     //         default:
            //     //             assert(!"invalid light type!");
            //     //         break;
            //     //     }

            //     //     data.lightDir = dparam.lightDir;
            //     //     data.lightColor = dparam.lightColor;
            //     // }

            //     if(Result::eSuccess != mContext->writeBuffer(sizeof(LightData), &data, mLightUB))
            //     {
            //         assert(!"failed to create light buffer!");
            //     }

            // }
        }

        //サブコマンドバッファ積み込み
        if(mShadowAdded)
        {
            mShadowAdded = false;
        }
        if(mGeometryAdded)
        {
            //assert(!"came");
            CommandList cl;
            cl.barrier(mGBuffer.albedoRT);
            cl.barrier(mGBuffer.normalRT);
            cl.barrier(mGBuffer.worldPosRT);
            cl.begin(mGBuffer.renderPass);
            for(const auto& sub : mGeometrySubs)
                cl.executeSubCommand(sub);
            cl.end();
            mContext->updateCommandBuffer(cl, mGeometryCB);
            mGeometryAdded = false;
        }
        if(mLightingAdded)
        {
            ColorClearValue ccv = {0.4f, 0.4f, 0.4f, 1.f};
            CommandList cl;
            cl.begin(mLightingPass, true, ccv);
            for(const auto& sub : mLightingSubs)
                cl.executeSubCommand(sub);
            cl.end();
            mContext->updateCommandBuffer(cl, mLightingCB);
            mLightingAdded = false;
        }
        // if(mForwardAdded)
        // {
        //     CommandList cl;
        //     cl.begin();
        //     for(const auto& sub : mLightingSubs)
        //         cl.executeSubCommand(sub);
        //     cl.end();
        //     mContext->updateCommandBuffer(cl, mLightingCB);
        // }
        if(mPostEffectAdded)
        {
            mPostEffectAdded = false;
        }
        if(mSpriteAdded)
        {
            mSpriteAdded = false;
        }

        mSceneBuilded = true;
    }

    void Renderer::render()
    {
        // if(!mSceneBuilded)
        // {
        //     build();
        //     mSceneBuilded = false;
        // }

        //mContext->execute(mShadowCB);
        std::cerr << "render start\n";
        mContext->execute(mGeometryCB);
        std::cerr << "geom\n";
        mContext->execute(mLightingCB);

        // for(const auto& cb : mLightingSubs)
        //     mContext->execute(cb);
        std::cerr << "light\n";

        //mContext->execute(mForwardCB);
        //mContext->execute(mPostEffectCB);
        //mContext->execute(mSpriteCB);

        for(const auto& cb : mPresentCBs)
            mContext->execute(cb);
        //assert(0);
    }
}