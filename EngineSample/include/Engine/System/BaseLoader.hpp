#pragma once

#include <memory>

class MeshComponent;
class MaterialComponent;

namespace Engine
{
    class BaseLoader
    {
    public:

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

    };
}