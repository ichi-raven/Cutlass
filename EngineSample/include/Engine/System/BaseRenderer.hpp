#pragma once

#include <Cutlass.hpp>

namespace Engine
{
    class MeshComponent;
    class MaterialComponent;
    class CameraComponent;
    class LightComponent;

    class BaseRenderer
    {
    public:
        BaseRenderer(){}

        void init(Cutlass::Context& context);

        //Noncopyable, Nonmoveable
        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer &operator=(const BaseRenderer&) = delete;
        BaseRenderer(BaseRenderer&&) = delete;
        BaseRenderer &operator=(BaseRenderer&&) = delete;

        virtual void addMesh
        (
            const std::shared_ptr<MeshComponent> mesh, 
            const std::shared_ptr<MaterialComponent> material
        );

        virtual void addCamera
        (
            const std::shared_ptr<CameraComponent> camera
        );

        virtual void addLight
        (
            const std::shared_ptr<LightComponent> light
        );

        virtual void render(const Cutlass::HRenderDST& windowRDST);

    private:

        Cutlass::HTexture mRTTex;
        Cutlass::HRenderDST mIntermediateDST;
        std::vector<Cutlass::CommandList> mCommandList;
        Cutlass::HCommandBuffer mCommandBuffer;
    };
};