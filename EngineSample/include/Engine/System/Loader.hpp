#pragma once

#include <memory>

namespace Engine
{
    class MeshComponent;
    class MaterialComponent;

    class Loader
    {
    public:

        Loader()
        : mIsLoaded(false)
        {}

        virtual void load(const char* path);
        virtual void load
        (
            const char* path,
            std::shared_ptr<MeshComponent> mesh_out,
            std::shared_ptr<MaterialComponent> material_out
        );

        virtual void getMesh(std::shared_ptr<MeshComponent> mesh_out);
        virtual void getMaterial(std::shared_ptr<MaterialComponent> material_out);

        virtual void unload();

    private:
        bool mIsLoaded;
    };
}