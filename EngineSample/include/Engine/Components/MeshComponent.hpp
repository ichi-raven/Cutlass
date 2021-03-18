#pragma once

#include <Cutlass.hpp>
#include <glm/glm.hpp>

#include "IComponent.hpp"

#include "../Utility/Transform.hpp"

namespace Engine
{
    class MeshComponent : public IComponent
    {
    public:
        //頂点構造, これをいじるとRendererとかも変わります
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec3 normal;
            glm::vec2 UV;
        };

        //VertexLayoutはMeshごとに固定です
        //上の型を変えたときに変えてください
        static Cutlass::VertexLayout getVertexLayout()
        {
            if(!meshVL)
            {
                Cutlass::VertexLayout vl;
                vl.set(Cutlass::ResourceType::eF32Vec3, "position");
                vl.set(Cutlass::ResourceType::eF32Vec3, "color");
                vl.set(Cutlass::ResourceType::eF32Vec3, "normal");
                vl.set(Cutlass::ResourceType::eF32Vec2, "uv");
                meshVL = vl;
            }
            return meshVL.value();
        }

        MeshComponent();
        virtual ~MeshComponent(){}

        //メッシュを構築する
        void createCube(Cutlass::Context& context, const double& edgeLength);
        void create
        (
            Cutlass::Context& context,
            const std::vector<MeshComponent::Vertex>& vertices,
            const std::vector<uint32_t>& indices
        );

        void setVisible(bool flag);
        bool getVisible() const;

        void setEnable(bool flag);
        bool getEnable() const;

        void setTransform(const Transform& transform);
        Transform& getTransform();
        //const Transform& getTransform() const;

        const uint32_t getVertexNum() const;
        const uint32_t getIndexNum() const;

        const Cutlass::HBuffer& getVB() const;

        const Cutlass::HBuffer& getIB() const;

        virtual void update() override;

    private:
        //メッシュで固定、自作Componentとか使う場合はどうにかしてください
        static inline std::optional<Cutlass::VertexLayout> meshVL;

        bool mVisible;
        bool mEnabled;
        Transform mTransform;

        std::vector<Vertex> mVertices;
        std::vector<uint32_t> mIndices;

        Cutlass::HBuffer mVB;
        Cutlass::HBuffer mIB;
    };
};