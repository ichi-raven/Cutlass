#include <Engine/System/BaseRenderer.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/MeshComponent.hpp>

BaseRenderer::BaseRenderer()
{
    auto& context = Cutlass::Context::getInstance();
    {
        Cutlass::TextureInfo ti;
        ti.setRTTex2D(1920, 1080);
        if(Cutlass::Result::eSuccess != context.createTexture(ti, mRTTex))
            return;
    }

    if(Cutlass::Result::eSuccess != context.createRenderDST(mRTTex, mIntermediateDST))
        return;
    

}

void BaseRenderer::addMesh(const std::shared_ptr<MeshComponent> mesh, const std::shared_ptr<MaterialComponent> material)
{
    
}