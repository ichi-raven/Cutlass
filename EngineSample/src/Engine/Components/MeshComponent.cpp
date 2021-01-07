#include <Engine/Components/MeshComponent.hpp>

#include <Engine/Components/MaterialComponent.hpp>

namespace Engine
{
    MeshComponent::MeshComponent()
    : mVisible(false)
    , mEnabled(false)
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

    const Transform& MeshComponent::getTransform() const
    {
        return mTransform;
    }

    const Cutlass::HBuffer& MeshComponent::getVB() const
    {
        return mVB;
    }

    const Cutlass::HBuffer& MeshComponent::getIB() const
    {
        return mIB;
    }

    void MeshComponent::update()
    {
        //update
        mTransform.update();
    }

    void MeshComponent::create(Cutlass::Context& context, const std::vector<MeshComponent::Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        mVisible = mEnabled = true;
        mVertices = vertices;
        mIndices = indices;

        {//頂点バッファ構築
            Cutlass::BufferInfo bi;
            bi.setVertexBuffer<Vertex>(mVertices.size());
            context.createBuffer(bi, mVB);
            context.writeBuffer(mVertices.size() * sizeof(decltype(mVertices[0])), mVertices.data(), mVB);
        }

        {//インデックスバッファ構築
            Cutlass::BufferInfo bi;
            bi.setIndexBuffer<uint32_t>(mIndices.size());
            context.createBuffer(bi, mIB);
            context.writeBuffer(mIndices.size() * sizeof(decltype(mIndices[0])), mIndices.data(), mVB);
        }
    }

    void MeshComponent::createCube(Cutlass::Context& context, const double& edgeLength)
    {
        mVisible = mEnabled = true;

        constexpr glm::vec3 red(1.0f, 0.0f, 0.0f);
        constexpr glm::vec3 green(0.0f, 1.0f, 0.0f);
        constexpr glm::vec3 blue(0.0f, 0.0f, 1.0f);
        constexpr glm::vec3 white(1.0f);
        constexpr glm::vec3 black(0.0f);
        constexpr glm::vec3 yellow(1.0f, 1.0f, 0.0f);
        constexpr glm::vec3 magenta(1.0f, 0.0f, 1.0f);
        constexpr glm::vec3 cyan(0.0f, 1.0f, 1.0f);

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

        mVertices = 
        {
            // 正面
            {glm::vec3(-edgeLength, edgeLength, edgeLength), yellow, nf, lb},
            {glm::vec3(-edgeLength, -edgeLength, edgeLength), red, nf, lt},
            {glm::vec3(edgeLength, edgeLength, edgeLength), white, nf, rb},
            {glm::vec3(edgeLength, -edgeLength, edgeLength), magenta, nf, rt},
            // 右
            {glm::vec3(edgeLength, edgeLength, edgeLength), white, nr, lb},
            {glm::vec3(edgeLength, -edgeLength, edgeLength), magenta, nr, lt},
            {glm::vec3(edgeLength, edgeLength, -edgeLength), cyan, nr, rb},
            {glm::vec3(edgeLength, -edgeLength, -edgeLength), blue, nr, rt},
            // 左
            {glm::vec3(-edgeLength, edgeLength, -edgeLength), green, nl, lb},
            {glm::vec3(-edgeLength, -edgeLength, -edgeLength), black, nl, lt},
            {glm::vec3(-edgeLength, edgeLength, edgeLength), yellow, nl, rb},
            {glm::vec3(-edgeLength, -edgeLength, edgeLength), red, nl, rt},
            // 裏
            {glm::vec3(edgeLength, edgeLength, -edgeLength), cyan, nb, lb},
            {glm::vec3(edgeLength, -edgeLength, -edgeLength), blue, nb, lt},
            {glm::vec3(-edgeLength, edgeLength, -edgeLength), green, nb, rb},
            {glm::vec3(-edgeLength, -edgeLength, -edgeLength), black, nb, rt},
            // 上
            {glm::vec3(-edgeLength, edgeLength, -edgeLength), green, nu, lb},
            {glm::vec3(-edgeLength, edgeLength, edgeLength), yellow, nu, lt},
            {glm::vec3(edgeLength, edgeLength, -edgeLength), cyan, nu, rb},
            {glm::vec3(edgeLength, edgeLength, edgeLength), white, nu, rt},
            // 底
            {glm::vec3(-edgeLength, -edgeLength, edgeLength), red, nd, lb},
            {glm::vec3(-edgeLength, -edgeLength, -edgeLength), black, nd, lt},
            {glm::vec3(edgeLength, -edgeLength, edgeLength), magenta, nd, rb},
            {glm::vec3(edgeLength, -edgeLength, -edgeLength), blue, nd, rt},
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

        {
            Cutlass::BufferInfo bi;
            bi.setVertexBuffer<Vertex>(mVertices.size());
            context.createBuffer(bi, mVB);
            context.writeBuffer(mVertices.size() * sizeof(decltype(mVertices[0])), mVertices.data(), mVB);
        }

        {
            Cutlass::BufferInfo bi;
            bi.setIndexBuffer<uint32_t>(mIndices.size());
            context.createBuffer(bi, mIB);
            context.writeBuffer(mIndices.size() * sizeof(decltype(mIndices[0])), mIndices.data(), mVB);
        }
    }
}