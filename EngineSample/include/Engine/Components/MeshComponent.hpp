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
        MeshComponent();
        //MeshComponent(const char* path);

        //マテリアル関係なしにメッシュをロードする
        void loadCube(const double& edgeLength);

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
        //頂点型は適当に定義してください(VLさえ渡せてシェーダと適合すればOK)
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec3 normal;
            glm::vec2 UV;
        };

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