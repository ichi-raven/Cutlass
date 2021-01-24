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
            mContext->createRenderPass(hw, mPresentPasses.emplace_back());

            TextureInfo ti;
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
            ti.setRTTex2D(width, height);
            mContext->createTexture(ti, mRTTexs.emplace_back());
        }
        
        mContext->createRenderPass(RenderPassCreateInfo(mRTTexs), mTexPasses.emplace_back());
    }

    Renderer::~Renderer()
    {

    }

    void Renderer::regist(const std::shared_ptr<MeshComponent> mesh, const std::shared_ptr<MaterialComponent> material)
    {
        auto&& tmp = mRenderInfos.emplace_back();
        tmp.mesh = mesh;
        tmp.material = material;
        //迷っている
        tmp.vertexBuffer = mesh->getVB();
        tmp.indexBuffer = mesh->getIB();
        
    }

    void Renderer::buildScene()
    {
        for(auto& geom : mRenderInfos)
        {
        //     GraphicsPipelineInfo gpi
        //     (
        //         geom.mesh->getVertexLayout(),
        //         ColorBlend::eDefault,
        //         Topology::eTriangleList,
        //         RasterizerState(PolygonMode::eFill, CullMode::eBack, FrontFace::eClockwise, 1.f),
        //         MultiSampleState::eDefault,
        //         DepthStencilState::eDepth,
        //         Shader("../Shaders/TexturedCube/vert.spv", "main"),
        //         Shader("../Shaders/TexturedCube/frag.spv", "main"),
        //         SRDesc,
        //         texPass
        //     );
        }
    }

    void Renderer::render()
    {
        //毎フレームRenderPipeline実体, CommandBuffer実体を構築して描画する
        //Cameraが無けりゃ描画はできません
        
    }

}