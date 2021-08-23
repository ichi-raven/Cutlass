#include <Engine/System/Renderer.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/CustomMaterialComponent.hpp>
#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/SkeletalMeshComponent.hpp>
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

        mMaxWidth = mMaxHeight = 0;

        for(const auto& hw : hwindows)
        {
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
            if(mMaxWidth < width)
                mMaxWidth = width;
            if(mMaxHeight < height)
                mMaxHeight = height;
        }

        {//シャドウマップ、パス
            Cutlass::TextureInfo ti;
            ti.setRTTex2DColor(mMaxWidth * 4, mMaxHeight * 4);
            if(Result::eSuccess != mContext->createTexture(ti, mShadowMap))
                assert(!"failed to create shadow map!");

             if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo(mShadowMap), mShadowPass))
                assert(!"failed to create shadow renderpass!");
        }

        {//G-Buffer 構築
            Cutlass::TextureInfo ti;
            constexpr float coef = 1;
            ti.setRTTex2DColor(mMaxWidth * coef, mMaxHeight * coef);
            mContext->createTexture(ti, mGBuffer.albedoRT);
            ti.format = Cutlass::ResourceType::eF32Vec4;
            mContext->createTexture(ti, mGBuffer.normalRT);
            mContext->createTexture(ti, mGBuffer.worldPosRT);
            ti.setRTTex2DDepth(mMaxWidth * coef, mMaxHeight * coef);
            mContext->createTexture(ti, mGBuffer.depthBuffer);

            if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo({mGBuffer.albedoRT, mGBuffer.normalRT, mGBuffer.worldPosRT}, mGBuffer.depthBuffer), mGBuffer.renderPass))
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
                    //SRSet[i].bind(0, mShadowMap);
                
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
        mShadowFS           = Shader("../resources/shaders/HLSLShadow/shadow_frag.spv", "PSMain");
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
                Topology::eTriangleStrip
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
            bi.setUniformBuffer<LightData>(MAX_LIGHT_NUM);
            if(Result::eSuccess != mContext->createBuffer(bi, mLightUB))
                assert(!"failed to create light UB!");
        }
        
        {//シャドウ用バッファ
            BufferInfo bi;
            bi.setUniformBuffer<ShadowData>();
            if(Result::eSuccess != mContext->createBuffer(bi, mShadowUB))
                assert(!"failed to create shadow UB!");
        }

        // {//シャドウマップ、パス
        //     Cutlass::TextureInfo ti;
        //     ti.setRTTex2DColor(mMaxWidth, mMaxHeight);
        //     if(Result::eSuccess != mContext->createTexture(ti, mShadowMap))
        //         assert(!"failed to create shadow map!");

        //      if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo(mShadowMap), mShadowPass))
        //         assert(!"failed to create shadow renderpass!");
        // }

        {//ライティング用コマンドバッファ
            ShaderResourceSet bufferSet, textureSet;
            {
                bufferSet.bind(0, mLightUB);
                bufferSet.bind(1, mCameraUB);
                bufferSet.bind(2, mShadowUB);

                textureSet.bind(0, mGBuffer.albedoRT);
                textureSet.bind(1, mGBuffer.normalRT);
                textureSet.bind(2, mGBuffer.worldPosRT);
                textureSet.bind(3, mShadowMap);
            }

            CommandList cl;
            cl.begin(mLightingPass);
            cl.bind(mLightingPipeline);
            cl.bind(0, bufferSet);
            cl.bind(1, textureSet);
            cl.render(4, 1, 0, 0);
            cl.end();
            mContext->updateCommandBuffer(cl, mLightingCB);
        }
    }

    Renderer::~Renderer()
    {
        
    }

    void Renderer::addStaticMesh(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<MaterialComponent>& material, bool lighting, bool castShadow, bool receiveShadow)
    {
        auto& tmp = mRenderInfos.emplace_back();
        tmp.skeletal = false;
        tmp.receiveShadow = receiveShadow;
        tmp.lighting = lighting;
        tmp.mesh = mesh;
        tmp.material = material;

        //頂点バッファ、インデックスバッファ構築
        {
            Cutlass::BufferInfo bi;
            bi.setVertexBuffer<MeshComponent::Vertex>(mesh->getVertexNum());
            mContext->createBuffer(bi, tmp.VB);
            mContext->writeBuffer(mesh->getVertices().size() * sizeof(MeshComponent::Vertex), mesh->getVertices().data(), tmp.VB);
        }

        {
            Cutlass::BufferInfo bi;
            bi.setIndexBuffer<uint32_t>(mesh->getIndexNum());
            mContext->createBuffer(bi, tmp.IB);
            mContext->writeBuffer(mesh->getIndices().size() * sizeof(uint32_t), mesh->getIndices().data(), tmp.IB);
        }

        //定数バッファ構築
        {
            Cutlass::BufferInfo bi;
            {//WVP
                bi.setUniformBuffer<SceneData>();
                mContext->createBuffer(bi, tmp.sceneCB);
            }

            {//ボーン
                //注意 : いまのところおいとく
                BoneData data;

                bi.setUniformBuffer<BoneData>();
                mContext->createBuffer(bi, tmp.boneCB);
                mContext->writeBuffer(sizeof(BoneData), &data, tmp.boneCB);
            }

        }

        if(castShadow)
        {//シャドウマップ用パス
        
            GraphicsPipelineInfo gpi
            (
                mShadowVS,
                mShadowFS,
                mShadowPass,
                DepthStencilState::eDepth,
                mesh->getRasterizerState(),
                mesh->getTopology()
            );

            if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.shadowPipeline))
                assert(!"failed to create shadow pipeline");
        }

        {
            GraphicsPipelineInfo gpi
            (
                mDefferedSkinVS,
                mDefferedSkinFS,
                mGBuffer.renderPass,
                DepthStencilState::eDepth,
                mesh->getRasterizerState(),
                mesh->getTopology()
            );
    
            if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.geometryPipeline))
                assert(!"failed to create geometry pipeline");
        }

        {//コマンド作成

            //シャドウパス
            if(castShadow)
            {
                ShaderResourceSet bufferSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    bufferSet.bind(1, mShadowUB);
                    bufferSet.bind(2, tmp.boneCB);
                }

                SubCommandList scl(mShadowPass);
                scl.bind(tmp.shadowPipeline);

                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.renderIndexed(mesh->getIndexNum(), 1, 0, 0, 0);

                HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, cb))
                    assert(!"failed to create command buffer!");
                mShadowSubs.emplace_back(cb);
            }

            //ジオメトリパス
            {
                ShaderResourceSet bufferSet;
                ShaderResourceSet textureSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    // if(!material->getMaterialSets().empty())
                    //     bufferSet.bind(1, material->getMaterialSets().back().paramBuffer);
                    // else
                    // {
                    //     //マテリアル読む
                    // }
                    bufferSet.bind(1, tmp.boneCB);

                    auto&& textures = material->getTextures();

                    if(textures.empty())
                        textureSet.bind(0, mDebugTex);
                    else
                        textureSet.bind(0, textures[0].handle);
                }

                SubCommandList scl(mGBuffer.renderPass);
                scl.bind(tmp.geometryPipeline);
                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.bind(1, textureSet);
                scl.renderIndexed(mesh->getIndexNum(), 1, 0, 0, 0);

                HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, cb))
                    assert(!"failed to create command buffer!");
                mGeometrySubs.emplace_back(cb);
            }
        }
        
        //影コントロール実装時注意
        mShadowAdded = true;
        mGeometryAdded = true;
        //std::cerr << "registed\n";
    }

    void Renderer::addCustom(const std::shared_ptr<MeshComponent>& mesh, const std::shared_ptr<CustomMaterialComponent>& material)
    {
        assert(!"TODO");
        mForwardAdded = true;
    }

    void Renderer::addSkeletalMesh(const std::shared_ptr<SkeletalMeshComponent>& skeletalMesh, const std::shared_ptr<MaterialComponent>& material, bool lighting, bool castShadow, bool receiveShadow)
    {
        auto& tmp = mRenderInfos.emplace_back();
        tmp.skeletal = true;
        tmp.receiveShadow = receiveShadow;
        tmp.lighting = lighting;
        tmp.mesh = static_cast<std::shared_ptr<MeshComponent>>(skeletalMesh);
        tmp.skeletalMesh = skeletalMesh;
        tmp.material = material;

        //頂点バッファ、インデックスバッファ構築
        {
            Cutlass::BufferInfo bi;
            bi.setVertexBuffer<MeshComponent::Vertex>(skeletalMesh->getVertexNum());
            mContext->createBuffer(bi, tmp.VB);
            mContext->writeBuffer(skeletalMesh->getVertices().size() * sizeof(MeshComponent::Vertex), skeletalMesh->getVertices().data(), tmp.VB);
        }

        {
            Cutlass::BufferInfo bi;
            bi.setIndexBuffer<uint32_t>(skeletalMesh->getIndexNum());
            mContext->createBuffer(bi, tmp.IB);
            mContext->writeBuffer(skeletalMesh->getIndices().size() * sizeof(uint32_t), skeletalMesh->getIndices().data(), tmp.IB);
        }

        //定数バッファ構築
        {
            Cutlass::BufferInfo bi;
            {//WVP
                bi.setUniformBuffer<SceneData>();
                mContext->createBuffer(bi, tmp.sceneCB);
            }

            {//ボーン
                //注意 : いまのところおいとく
                //BoneData data;

                bi.setUniformBuffer<BoneData>();
                mContext->createBuffer(bi, tmp.boneCB);
                //mContext->writeBuffer(sizeof(BoneData), &data, tmp.boneCB);
            }

        }

        if(castShadow)
        {//シャドウマップ用パス
            GraphicsPipelineInfo gpi
            (
                mShadowVS,
                mShadowFS,
                mShadowPass,
                DepthStencilState::eDepth,
                skeletalMesh->getRasterizerState(),
                skeletalMesh->getTopology()
            );

            if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.shadowPipeline))
                assert(!"failed to create shadow pipeline");
        }

        {
            GraphicsPipelineInfo gpi
            (
                mDefferedSkinVS,
                mDefferedSkinFS,
                mGBuffer.renderPass,
                DepthStencilState::eDepth,
                skeletalMesh->getRasterizerState(),
                skeletalMesh->getTopology()
            );
    
            if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.geometryPipeline))
                assert(!"failed to create geometry pipeline");
        }

        {//コマンド作成

            //シャドウパス
            if(castShadow)
            {
                ShaderResourceSet bufferSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    bufferSet.bind(1, mShadowUB);
                    bufferSet.bind(2, tmp.boneCB);
                }

                SubCommandList scl(mShadowPass);
                scl.bind(tmp.shadowPipeline);

                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.renderIndexed(skeletalMesh->getIndexNum(), 1, 0, 0, 0);

                HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, cb))
                    assert(!"failed to create command buffer!");
                mShadowSubs.emplace_back(cb);
            }

            //ジオメトリパス
            {
                ShaderResourceSet bufferSet;
                ShaderResourceSet textureSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    // if(!material->getMaterialSets().empty())
                    //     bufferSet.bind(1, material->getMaterialSets().back().paramBuffer);
                    // else
                    // {
                    //     //マテリアル読む
                    // }
                    bufferSet.bind(1, tmp.boneCB);

                    auto&& textures = material->getTextures();

                    if(textures.empty())
                        textureSet.bind(0, mDebugTex);
                    else
                        textureSet.bind(0, textures[0].handle);
                }

                SubCommandList scl(mGBuffer.renderPass);
                scl.bind(tmp.geometryPipeline);
                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.bind(1, textureSet);
                scl.renderIndexed(skeletalMesh->getIndexNum(), 1, 0, 0, 0);

                HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, cb))
                    assert(!"failed to create command buffer!");
                mGeometrySubs.emplace_back(cb);
            }
        }
        
        //影コントロール実装時注意
        mShadowAdded = true;
        mGeometryAdded = true;
    }

    void Renderer::addLight(const std::shared_ptr<LightComponent>& light)
    {
        //std::cerr << "light start\n";
        mLights.emplace_back(light);

        //shadow用
        {
            ShadowData data;
            auto view = glm::lookAtRH(light->getDirection() * -20.f, glm::vec3(0, 0, 0), glm::vec3(0, 1.f, 0));
            auto proj = glm::perspective(glm::radians(60.f), 1.f * mMaxWidth / mMaxHeight, 1.f, 1000.f);
            //auto proj = glm::ortho(-10.f, 10.0f, -5.0f, 30.0f, 0.5f, 50.0f);
            proj[1][1] *= -1;
            auto&& matBias =  glm::translate(glm::mat4(1.0f), glm::vec3(0.5f,0.5f,0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
            data.lightViewProj = proj * view;
            data.lightViewProjBias = matBias * data.lightViewProj;
            //data.lightViewProj = glm::perspective(glm::radians(45.f), 1.f * width / height, 1.f, 1000.f) * glm::lookAtRH(glm::vec3(0), glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
            mContext->writeBuffer(sizeof(ShadowData), &data, mShadowUB);
        }

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

            mContext->destroyBuffer(ri.sceneCB);
            mContext->destroyGraphicsPipeline(ri.geometryPipeline);
            mContext->destroyGraphicsPipeline(ri.shadowPipeline);
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
                sceneData.receiveShadow = ri.receiveShadow ? 1.f : 0;
                sceneData.lighting = ri.lighting ? 1.f : 0;
                mContext->writeBuffer(sizeof(SceneData), &sceneData, ri.sceneCB);

                if(!ri.skeletal)
                    continue;

                BoneData data;
                const auto& bones = ri.skeletalMesh->getBones();
                const auto&& identity = glm::mat4(0.f);
                data.useBone = 1;
                for(size_t i = 0; i < MAX_BONE_NUM; ++i)
                {
                    if(i >= bones.size())
                    {
                        data.boneTransform[i] = identity;
                        continue;
                    }

                    data.boneTransform[i] = bones[i].transform;
                }
                mContext->writeBuffer(sizeof(BoneData), &data, ri.boneCB);

            }

            {//カメラ
                CameraData data;
                data.cameraPos = mCamera.value()->getTransform().getPos();
                //std::cerr << "camera pos : " << glm::to_string(data.cameraPos) << "\n";
                mContext->writeBuffer(sizeof(CameraData), &data, mCameraUB);
            }

            {//ライト
                LightData data[MAX_LIGHT_NUM];

                for(uint32_t i = 0; i < MAX_LIGHT_NUM; ++i)
                {
                    if(i >= mLights.size())
                    {
                        data[i].lightType = 0;
                        data[i].lightColor = glm::vec4(0);
                        data[i].lightDir = glm::vec3(0);
                        continue;
                    }

                    switch(mLights[i]->getType())
                    {
                        case LightComponent::LightType::eDirectionalLight:
                            data[i].lightType = 0;
                        break;
                        case LightComponent::LightType::ePointLight:
                            data[i].lightType = 1;
                            assert(!"TODO");
                        break;
                        default:
                            assert(!"invalid light type!");
                        break;
                    }
                    data[i].lightColor = mLights[i]->getColor();
                    data[i].lightDir = mLights[i]->getDirection();
                }

                // std::cerr << sizeof(LightData) << "\n";
                // assert(0);

                if(Result::eSuccess != mContext->writeBuffer(sizeof(LightData) * MAX_LIGHT_NUM, &data, mLightUB))
                {
                    assert(!"failed to create light buffer!");
                }
            }

        }

        //サブコマンドバッファ積み込み
        if(mShadowAdded)
        {
            CommandList cl;

            cl.begin(mShadowPass);
            for(const auto& sub : mShadowSubs)
                cl.executeSubCommand(sub);
            cl.end();
            mContext->updateCommandBuffer(cl, mShadowCB);
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
        if(!mSceneBuilded)
        {
            assert(!"scene wasn't builded!");
            return;
        }

        std::cerr << "render start\n";
        mContext->execute(mShadowCB);
        //std::cerr << "shadow\n";
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
    }
}