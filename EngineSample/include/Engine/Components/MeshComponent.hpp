#pragma once

#include <Cutlass/Cutlass.hpp>
#include <glm/glm.hpp>

#include "IComponent.hpp"

class MeshComponent : public IComponent
{
public:
    MeshComponent();
    MeshComponent(const char* path);

    void loadCube(const double& edgeLength);

    void load(const char* path);

    void setVisible(bool flag);
    bool getVisible() const;

    const Cutlass::HBuffer& getVB() const;

    const Cutlass::HBuffer& getIB() const;

    const Cutlass::VertexLayout& getVL() const;

    virtual void update() override;

private:
    //頂点型は適当に定義すべきです
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 UV;
    };

    bool mVisible;

    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;

    Cutlass::HBuffer mVB;
    Cutlass::HBuffer mIB;

    Cutlass::VertexLayout mVertexLayout;
};