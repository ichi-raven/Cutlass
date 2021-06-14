#pragma once

#include <Cutlass.hpp>
#include <glm/glm.hpp>

#include <typeinfo>

#include "IComponent.hpp"

#include "../Utility/Transform.hpp"


namespace Engine
{
    class MeshComponent : public IComponent
    {
    public:
        //頂点構造, これをいじるとRendererとかも変わります
        #define EQ(MEMBER) (MEMBER == another.MEMBER)//便利
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec4 color;
            glm::vec3 normal;
            glm::vec2 UV;

            bool operator==(const Vertex& another) const 
            {
                return EQ(pos) && EQ(color) && EQ(normal) && EQ(UV);
            }
        };

        struct CPUVertex
        {
            glm::vec3 pos;

            bool operator==(const CPUVertex& another) const 
            {
                return EQ(pos);
            }
        };

        struct GLTFVertex
        {
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec2 uv0;
            glm::vec2 uv1;
            glm::vec4 joint0;
            glm::vec4 weight0;

            bool operator==(const GLTFVertex& another) const 
            {
                return EQ(pos) && EQ(normal) && EQ(uv0) && EQ(uv1) && EQ(joint0) && EQ(weight0);
            }
        };

        MeshComponent();
        virtual ~MeshComponent();

        //メッシュを構築する
        template<typename VertexType>
        void create
        (
            const std::vector<VertexType>& vertices,
            const std::vector<uint32_t>& indices
        )
        {
            auto&& context = getContext();
            mVisible = mEnabled = true;
            {//ボトルネック
                mVertices.resize(vertices.size());
                for(uint32_t i = 0; i < mVertices.size(); ++i)
                    mVertices[i].pos = vertices[i].pos;
            }

            mIndices = indices;

            {//頂点バッファ構築
                Cutlass::BufferInfo bi;
                bi.setVertexBuffer<Vertex>(vertices.size());
                context->createBuffer(bi, mVB);
                context->writeBuffer(vertices.size() * sizeof(VertexType), vertices.data(), mVB);
            }

            {//インデックスバッファ構築
                Cutlass::BufferInfo bi;
                bi.setIndexBuffer<uint32_t>(mIndices.size());
                context->createBuffer(bi, mIB);
                context->writeBuffer(mIndices.size() * sizeof(decltype(mIndices[0])), mIndices.data(), mIB);
            }
        }

        //基本スタティックメッシュ構築
        void createCube(const double& edgeLength);
        void createPlane(const double& xSize, const double& zSize);
        
        void setVisible(bool flag);
        bool getVisible() const;

        void setEnable(bool flag);
        bool getEnable() const;

        void setTransform(const Transform& transform);
        Transform& getTransform();
        //const Transform& getTransform() const;

        void setTopology(Cutlass::Topology topology);
        Cutlass::Topology getTopology() const;

        void setRasterizerState(const Cutlass::RasterizerState& rasterizerState);
        const Cutlass::RasterizerState& getRasterizerState() const;

        const uint32_t getVertexNum() const;
        const uint32_t getIndexNum() const;

        const Cutlass::HBuffer& getVB() const;

        const Cutlass::HBuffer& getIB() const;

        virtual void update() override;

    private:

        bool mVisible;
        bool mEnabled;
        Transform mTransform;

        std::vector<CPUVertex> mVertices;
        std::vector<uint32_t> mIndices;

        Cutlass::Topology mTopology;
        Cutlass::RasterizerState mRasterizerState;

        Cutlass::HBuffer mVB;
        Cutlass::HBuffer mIB;
    };
};