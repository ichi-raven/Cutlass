#include <Engine/Components/MeshComponent.hpp>

#include <Engine/Components/MaterialComponent.hpp>

#include <iostream>

namespace Engine
{
    MeshComponent::MeshComponent()
    : mVisible(false)
    , mEnabled(false)
    {
        mTopology = Cutlass::Topology::eTriangleList;
        mRasterizerState = Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eClockwise);
    }

    MeshComponent::~MeshComponent()
    {

    }

    void MeshComponent::setVisible(bool flag)
    {
        mVisible = flag;
    }

    bool MeshComponent::getVisible() const
    {
        return mVisible;
    }

    void MeshComponent::setEnable(bool flag)
    {
        mEnabled = flag;
    }

    bool MeshComponent::getEnable() const
    {
        return mEnabled;
    }

    void MeshComponent::setTransform(const Transform& transform)
    {
        mTransform = transform;
    }

    Transform& MeshComponent::getTransform()
    {
        return mTransform;
    }

    const uint32_t MeshComponent::getVertexNum() const
    {
        return mVertices.size();
    }

    const uint32_t MeshComponent::getIndexNum() const
    {
        return mIndices.size();
    }

    // const Cutlass::HBuffer& MeshComponent::getVB() const
    // {
    //     return mVB;
    // }

    // const Cutlass::HBuffer& MeshComponent::getIB() const
    // {
    //     return mIB;
    // }

    const std::vector<MeshComponent::Vertex>& MeshComponent::getVertices() const
    {
        return mVertices;
    }

    const std::vector<uint32_t>& MeshComponent::getIndices() const
    {
        return mIndices;
    } 

    void MeshComponent::update()
    {
        //update
        mTransform.update();
    }

    void MeshComponent::setTopology(Cutlass::Topology topology)
    {
        mTopology = topology;
    }

    Cutlass::Topology MeshComponent::getTopology() const
    {
        return mTopology;
    }
    
    void MeshComponent::setRasterizerState(const Cutlass::RasterizerState& rasterizerState)
    {
        mRasterizerState = rasterizerState;
    }

    const Cutlass::RasterizerState& MeshComponent::getRasterizerState() const
    {
        return mRasterizerState;
    }

    void MeshComponent::create(const std::vector<MeshComponent::Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        //auto& context = getContext();
        mVisible = mEnabled = true;

        mIndices = indices;

        // {//頂点バッファ構築
        //     Cutlass::BufferInfo bi;
        //     bi.setVertexBuffer<Vertex>(mVertices.size());
        //     context->createBuffer(bi, mVB);
        //     context->writeBuffer(mVertices.size() * sizeof(decltype(mVertices[0])), mVertices.data(), mVB);
        // }

        // {//インデックスバッファ構築
        //     Cutlass::BufferInfo bi;
        //     bi.setIndexBuffer<uint32_t>(mIndices.size());
        //     context->createBuffer(bi, mIB);
        //     context->writeBuffer(mIndices.size() * sizeof(decltype(mIndices[0])), mIndices.data(), mIB);
        // }
    }

    void MeshComponent::createCube(const double& edgeLength)
    {
        mVisible = mEnabled = true;

        constexpr glm::vec4 zero = glm::vec4(0, 0, 0, 0);

        // constexpr glm::vec4 red(1.0f, 0.0f, 0.0f, 1.f);
        // constexpr glm::vec4 green(0.0f, 1.0f, 0.0f, 1.f);
        // constexpr glm::vec4 blue(0.0f, 0.0f, 1.0f, 1.f);
        // constexpr glm::vec4 white(1.0f);
        // constexpr glm::vec4 black(0.0f);
        // constexpr glm::vec4 yellow(1.0f, 1.0f, 0.0f, 1.f);
        // constexpr glm::vec4 magenta(1.0f, 0.0f, 1.0f, 1.f);
        // constexpr glm::vec4 cyan(0.0f, 1.0f, 1.0f, 1.f);

        constexpr glm::vec2 lb(0.0f, 0.0f);
        constexpr glm::vec2 lt(0.0f, 1.0f);
        constexpr glm::vec2 rb(1.0f, 0.0f);
        constexpr glm::vec2 rt(1.0f, 1.0f);

        constexpr glm::vec3 nf(0, 0, 1.f);
        constexpr glm::vec3 nb(0, 0, -1.f);
        constexpr glm::vec3 nr(1.f, 0, 0);
        constexpr glm::vec3 nl(-1.f, 0, 0);
        constexpr glm::vec3 nu(0, 1.f, 0);
        constexpr glm::vec3 nd(0, -1.f, 0);

        mVertices.resize(24);
        mIndices.resize(36);

        mVertices = 
        {
            // 正面
            {glm::vec3(-edgeLength, edgeLength, edgeLength), nf, lb, zero, zero},
            {glm::vec3(-edgeLength, -edgeLength, edgeLength), nf, lt, zero, zero},
            {glm::vec3(edgeLength, edgeLength, edgeLength), nf, rb, zero, zero},
            {glm::vec3(edgeLength, -edgeLength, edgeLength), nf, rt, zero, zero},
            // 右
            {glm::vec3(edgeLength, edgeLength, edgeLength), nr, lb, zero, zero},
            {glm::vec3(edgeLength, -edgeLength, edgeLength), nr, lt, zero, zero},
            {glm::vec3(edgeLength, edgeLength, -edgeLength), nr, rb, zero, zero},
            {glm::vec3(edgeLength, -edgeLength, -edgeLength), nr, rt, zero, zero},
            // 左
            {glm::vec3(-edgeLength, edgeLength, -edgeLength), nl, lb, zero, zero},
            {glm::vec3(-edgeLength, -edgeLength, -edgeLength), nl, lt, zero, zero},
            {glm::vec3(-edgeLength, edgeLength, edgeLength), nl, rb, zero, zero},
            {glm::vec3(-edgeLength, -edgeLength, edgeLength), nl, rt, zero, zero},
            // 裏
            {glm::vec3(edgeLength, edgeLength, -edgeLength), nb, lb, zero, zero},
            {glm::vec3(edgeLength, -edgeLength, -edgeLength), nb, lt, zero, zero},
            {glm::vec3(-edgeLength, edgeLength, -edgeLength), nb, rb, zero, zero},
            {glm::vec3(-edgeLength, -edgeLength, -edgeLength), nb, rt, zero, zero},
            // 上
            {glm::vec3(-edgeLength, edgeLength, -edgeLength), nu, lb, zero, zero},
            {glm::vec3(-edgeLength, edgeLength, edgeLength), nu, lt, zero, zero},
            {glm::vec3(edgeLength, edgeLength, -edgeLength), nu, rb, zero, zero},
            {glm::vec3(edgeLength, edgeLength, edgeLength), nu, rt, zero, zero},
            // 底
            {glm::vec3(-edgeLength, -edgeLength, edgeLength), nd, lb, zero, zero},
            {glm::vec3(-edgeLength, -edgeLength, -edgeLength), nd, lt, zero, zero},
            {glm::vec3(edgeLength, -edgeLength, edgeLength), nd, rb, zero, zero},
            {glm::vec3(edgeLength, -edgeLength, -edgeLength), nd, rt, zero, zero},
        };

        mIndices =
        {
            0, 2, 1, 1, 2, 3,    // front
            4, 6, 5, 5, 6, 7,    // right
            8, 10, 9, 9, 10, 11, // left

            12, 14, 13, 13, 14, 15, // back
            16, 18, 17, 17, 18, 19, // top
            20, 22, 21, 21, 22, 23, // bottom
        };

    }

    void MeshComponent::createPlane(const double& xSize, const double& zSize)
    {
        mVisible = mEnabled = true;
        mRasterizerState.cullMode = Cutlass::CullMode::eNone;
        mRasterizerState.frontFace = Cutlass::FrontFace::eCounterClockwise;

        constexpr glm::vec3 nu(0, 1.f, 0);
        constexpr glm::vec2 lb(0.0f, 0.0f);
        constexpr glm::vec2 lt(0.0f, 1.0f);
        constexpr glm::vec2 rb(1.0f, 0.0f);
        constexpr glm::vec2 rt(1.0f, 1.0f);

        constexpr glm::vec4 zero = glm::vec4(0, 0, 0, 0);

        // constexpr glm::vec4 red(1.0f, 0.0f, 0.0f, 1.f);
        // constexpr glm::vec4 green(0.0f, 1.0f, 0.0f, 1.f);
        // constexpr glm::vec4 blue(0.0f, 0.0f, 1.0f, 1.f);
        // constexpr glm::vec4 yellow(1.0f, 1.0f, 0.0f, 1.f);

        mVertices.resize(4);
        mIndices.resize(6);

        mVertices = 
        {
            {glm::vec3(-xSize, 0, zSize), nu, lb, zero, zero},
            {glm::vec3(-xSize, 0, -zSize), nu, lt, zero, zero},
            {glm::vec3(xSize, 0, zSize), nu, rb, zero, zero},
            {glm::vec3(xSize, 0, -zSize), nu, rt, zero, zero}
        };

        mIndices = 
        {
            0, 2, 1, 1, 2, 3
        };

        // auto&& context = getContext();
        // {
        //     Cutlass::BufferInfo bi;
        //     bi.setVertexBuffer<Vertex>(vertices.size());
        //     context->createBuffer(bi, mVB);
        //     context->writeBuffer(vertices.size() * sizeof(decltype(vertices[0])), vertices.data(), mVB);
        // }

        // {
        //     Cutlass::BufferInfo bi;
        //     bi.setIndexBuffer<uint32_t>(mIndices.size());
        //     context->createBuffer(bi, mIB);
        //     context->writeBuffer(mIndices.size() * sizeof(decltype(mIndices[0])), mIndices.data(), mIB);
        // }
    }
}