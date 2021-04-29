#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "../ThirdParty/tiny_obj_loader.h"
#include "../ThirdParty/tiny_gltf.h"

namespace Cutlass
{
    class Context;
}

namespace Engine
{
    class MeshComponent;
    class MaterialComponent;

    class Loader
    {
    public:

        Loader(const std::shared_ptr<Cutlass::Context>& context)
        : mMode(Mode::eNone)
        , mContext(context)
        {}

        virtual void load(const char* path);
        virtual void load
        (
            const char* path,
            std::shared_ptr<MeshComponent>& mesh_out,
            std::shared_ptr<MaterialComponent>& material_out
        );

        virtual void getMesh(std::shared_ptr<MeshComponent>& mesh_out);
        virtual void getMaterial(std::shared_ptr<MaterialComponent>& material_out);

        virtual void unload();

    private:
        struct ObjModel
        {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
        };

        enum class Mode
        {
            eObj,
            eGLTF,
            eNone
        };


        std::variant<ObjModel, tinygltf::Model> mModel;
        Mode mMode;
        std::shared_ptr<Cutlass::Context> mContext;
    };
}