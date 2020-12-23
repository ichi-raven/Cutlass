#pragma once

#include <Cutlass/Cutlass.hpp>
#include <glm/glm.hpp>

#include "IComponent.hpp"

#include "../Utility/Transform.hpp"

namespace Engine
{
    class MeshComponent : public IComponent
    {
    public:
        //頂点構造
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec3 normal;
            glm::vec2 UV;
        };

        MeshComponent(Cutlass::Context& context);
        //MeshComponent(const char* path);

        //メッシュを構築する
        void createCube(const double& edgeLength);

        void create
        (
            const std::vector<MeshComponent::Vertex>& vertices,
            const std::vector<uint32_t>& indices
        );

        void setVisible(bool flag);
        bool getVisible() const;

        void setEnabled(bool flag);
        bool getEnabled() const;

        void setTransform(const Transform& transform);
        Transform& getTransform();
        const Transform& getTransform() const;

        const Cutlass::HBuffer& getVB() const;

        const Cutlass::HBuffer& getIB() const;

        const Cutlass::VertexLayout& getVL() const;

        virtual void update() override;

    private:
        Cutlass::Context& mContext;

        bool mVisible;
        bool mEnabled;
        Transform mTransform;

        std::vector<Vertex> mVertices;
        std::vector<uint32_t> mIndices;

        Cutlass::HBuffer mVB;
        Cutlass::HBuffer mIB;

        Cutlass::VertexLayout mVertexLayout;
    };
};