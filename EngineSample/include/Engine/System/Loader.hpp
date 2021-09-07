#pragma once

#include <memory>
#include <variant>
#include <vector>

// #include "../ThirdParty/tiny_obj_loader.h"
// #include "../ThirdParty/tiny_gltf.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/SkeletalMeshComponent.hpp>
#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/SpriteComponent.hpp>


namespace Cutlass
{
    class Context;
}

namespace Engine
{

    class Loader
    {
    public:

        struct Mesh 
	    {
            std::vector<MeshComponent::Vertex> vertices;
            std::vector<uint32_t> indices;
        };

        struct VertexBoneData
        {
            VertexBoneData()
            {
                weights[0] = weights[1] = weights[2] = weights[3] = 0;
                id[0] = id[1] = id[2] = id[3] = 0;
            }

            float weights[4];
            float id[4];
        };

        Loader(const std::shared_ptr<Cutlass::Context>& context)
        : mContext(context)
        , mBoneNum(0)
        {}

        virtual ~Loader();

        //virtual void load(const char* path);
        virtual void loadStatic
        (
            const char* path,
            std::shared_ptr<MeshComponent>& mesh_out,
            std::shared_ptr<MaterialComponent>& material_out
        );

        virtual void loadSkeletal
        (
            const char* path,
            std::shared_ptr<SkeletalMeshComponent>& skeletalMesh_out,
            std::shared_ptr<MaterialComponent>& material_out
        );

        //if type was nullptr, type will be last section of path
        virtual void loadMaterialTexture(const char* path, const char* type, std::shared_ptr<MaterialComponent>& material_out);

        virtual void loadSprite(const char* path, std::shared_ptr<SpriteComponent>& sprite_out);
        virtual void loadSprite(std::vector<const char*> pathes, std::shared_ptr<SpriteComponent>& sprite_out);

        //if type was nullptr, type will be last section of path

        // void loadObj
        // (
        //     const char* path,
        //     std::shared_ptr<MeshComponent>& mesh_out,
        //     std::shared_ptr<MaterialComponent>& material_out
        // );

        // void loadGLTF
        // (
        //     const char* path,
        //     std::shared_ptr<MeshComponent>& mesh_out,
        //     std::shared_ptr<MaterialComponent>& material_out
        // );

        virtual void unload();

    private:
        void processNode(const aiNode* node);

        Mesh processMesh(const aiNode* node, const aiMesh* mesh);

        std::vector<MaterialComponent::Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

        void loadBones(const aiNode* node, const aiMesh* mesh, std::vector<VertexBoneData>& vbdata_out);

        MaterialComponent::Texture loadTexture(const char* path, const char* type);
        
        
        // struct ObjModel
        // {
        //     tinyobj::attrib_t attrib;
        //     std::vector<tinyobj::shape_t> shapes;
        //     std::vector<tinyobj::material_t> materials;
        // };

        // enum class Mode
        // {
        //     eObj,
        //     eGLTF,
        //     eNone
        // };

        // std::variant<ObjModel, tinygltf::Model> mModel;
        // Mode mMode;
        
        std::shared_ptr<Cutlass::Context> mContext;
        bool mLoaded;
        bool mSkeletal;
        std::string mDirectory;
        std::string mPath;

        std::vector<Mesh> mMeshes;
        SkeletalMeshComponent::Skeleton mSkeleton;//単体前提
        //std::vector<SkeletalMeshComponent::Bone> mBones;
		//std::map<std::string, size_t> mBoneMap;

        //glm::mat4 mGlobalInverse;

        std::vector<MaterialComponent::Texture> mTexturesLoaded;


        uint32_t mBoneNum;

        Assimp::Importer mImporter;
        std::shared_ptr<const aiScene> mScene;
    };
}