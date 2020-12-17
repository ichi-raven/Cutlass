#pragma once

#include <Cutlass.hpp>

class MeshComponent;
class MaterialComponent;

class BaseRenderer
{
public:
    BaseRenderer();
    
    //Noncopyable
    BaseRenderer(const BaseRenderer&) = delete;
    BaseRenderer &operator=(const BaseRenderer&) = delete;
    BaseRenderer(BaseRenderer&&) = delete;
    BaseRenderer &operator=(BaseRenderer&&) = delete;

    virtual void addMesh(const std::shared_ptr<MeshComponent> mesh);
    
    virtual void addMesh(const std::shared_ptr<MeshComponent> mesh, const std::shared_ptr<MaterialComponent> material);

    virtual void render(const Cutlass::HWindow& window);

private:

    Cutlass::HTexture mRTTex;
    Cutlass::HRenderDST mIntermediateDST;
    std::vector<Cutlass::CommandList> mCommandList;
    Cutlass::HCommandBuffer mCommandBuffer;
};