#include <Engine/System/Loader.hpp>

// #define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION
// #define JSON_NOEXCEPTION
// #define TINYOBJLOADER_IMPLEMENTATION
// #include <Engine/ThirdParty/tiny_obj_loader.h>
// #include <Engine/ThirdParty/tiny_gltf.h>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/MaterialComponent.hpp>

#include <iostream>
#include <unordered_map>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>


//for extract unique vertex
// namespace std 
// {
//     template<> struct hash<Engine::MeshComponent::Vertex>
//     {
//         size_t operator()(Engine::MeshComponent::Vertex const& vertex) const 
//         {
//             return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
//         }
//     };
// }

namespace Engine
{
    //inline const glm::vec4 real3ToVec4(const tinyobj::real_t src[], const float w = 1.f)
    // {
    //     return glm::vec4(src[0], src[1], src[2], w);
    // };

    // inline const glm::vec4 real4ToVec4(const tinyobj::real_t src[])
    // {
    //     return glm::vec4(src[0], src[1], src[2], src[4]);
    // };

    // inline bool ends_with(const std::string& str, const std::string& suffix) 
    // {
    //     size_t len1 = str.size();
    //     size_t len2 = suffix.size();
    //     return len1 >= len2 && str.compare(len1 - len2, len2, suffix) == 0;
    // }

    inline glm::mat4 convert4x4(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        //transpose
        for(uint8_t i = 0; i < 4; ++i)
            for(uint8_t j = 0; j < 4; ++j)
                to[i][j] = from[j][i];
        

        return to;
    } 

    inline glm::quat convertQuat(const aiQuaternion& from)
    {
        glm::quat to;
        to.w = from.w;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;

        return to;
    }

    inline glm::vec3 convertVec3(const aiVector3D& from)
    {
        glm::vec3 to;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;

        return to;
    }

    inline glm::vec2 convertVec2(const aiVector2D& from)
    {
        glm::vec2 to;
        to.x = from.x;
        to.y = from.y;

        return to;
    }

    Loader::~Loader()
    {
        if(mLoaded)
            unload();
        
    }
    void Loader::unload()
    {
        mMeshes.clear();
        for(const auto t : mTexturesLoaded)
        {
            mContext->destroyTexture(t.handle);
        }
        mTexturesLoaded.clear();
        mSkeleton.bones.clear();
        mSkeleton.boneMap.clear();
        mLoaded = false;
    }

    void Loader::loadStatic
    (
        const char* modelPath,
        std::shared_ptr<MeshComponent>& mesh_out,
        std::shared_ptr<MaterialComponent>& material_out
    )
    {
        if(mLoaded)
            unload();

        mSkeletal = false;

        mPath = std::string(modelPath);

        mScene = std::shared_ptr<const aiScene>(mImporter.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs));

        if(!mScene || mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !mScene->mRootNode) 
        {
            std::cerr << "ERROR::ASSIMP::" << mImporter.GetErrorString() << "\n";
            assert(0);
            return;
        }

        mDirectory = mPath.substr(0, mPath.find_last_of("/\\"));

        // mSkeleton.setGlobalInverse(glm::inverse(convert4x4(mScene->mRootNode->mTransformation)));
        // mSkeleton.setAIScene(mScene);

        processNode(mScene->mRootNode);

        std::cerr << "bone num : " << mBoneNum << "\n";
        //assert(0);

        std::vector<MeshComponent::Vertex> vertices;
        std::vector<uint32_t> indices;

        vertices.reserve(1e5);
        indices.reserve(1e5);
        for(const auto& mesh : mMeshes)
        {
            std::cerr << "mesh vertix num : " << mesh.vertices.size() << "\n";
            std::copy(mesh.vertices.begin(), mesh.vertices.end(), std::back_inserter(vertices));
            std::copy(mesh.indices.begin(), mesh.indices.end(), std::back_inserter(indices));
        }

        mesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));
        mesh_out->create(vertices, indices);

        // if(mMode != Mode::eNone)
        //     unload();

        // if(ends_with(path, ".obj"))
        // {
        //     loadObj(modelPath, mesh_out, material_out);
        // }
        // else if(ends_with(path, ".gltf") || ends_with(path, ".vrm"))
        // {
        //     loadGLTF(modelPath, mesh_out, material_out);
        // }
        // else
        // {
        //     std::cerr << "load error : " << path << "\n";
        //     assert(!"This format is not supported!");
        //     mMode = Mode::eNone;
        // }
    }

    void Loader::loadSkeletal
    (
        const char* modelPath,
        std::shared_ptr<SkeletalMeshComponent>& skeletalMesh_out,
        std::shared_ptr<MaterialComponent>& material_out
    )
    {
        mSkeletal = true;
        if(mLoaded)
            unload();

        mPath = std::string(modelPath);

        mScene = std::shared_ptr<const aiScene>(mImporter.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs));

        if(!mScene || mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !mScene->mRootNode) 
        {
            std::cerr << "ERROR::ASSIMP::" << mImporter.GetErrorString() << "\n";
            assert(0);
            return;
        }

        mDirectory = mPath.substr(0, mPath.find_last_of("/\\"));

        mSkeleton.setGlobalInverse(glm::inverse(convert4x4(mScene->mRootNode->mTransformation)));
	    mSkeleton.setAIScene(mScene);

        processNode(mScene->mRootNode);

        std::cerr << "bone num : " << mBoneNum << "\n";
        //assert(0);

        std::vector<MeshComponent::Vertex> vertices;
        std::vector<uint32_t> indices;

        vertices.reserve(1e5);
        indices.reserve(1e5);
        for(const auto& mesh : mMeshes)
        {
            std::cerr << "mesh vertix num : " << mesh.vertices.size() << "\n";
            std::copy(mesh.vertices.begin(), mesh.vertices.end(), std::back_inserter(vertices));
            std::copy(mesh.indices.begin(), mesh.indices.end(), std::back_inserter(indices));
        }

        skeletalMesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));
        skeletalMesh_out->create(vertices, indices, mSkeleton);

        material_out->addTextures(mTexturesLoaded);
    }

    void Loader::processNode(const aiNode* node)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];
            if(mesh)
                mMeshes.emplace_back(processMesh(node, mesh));
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) 
        {
            processNode(node->mChildren[i]);
        }

    }

    Loader::Mesh Loader::processMesh(const aiNode* node, const aiMesh* mesh)
    {
        std::cerr << "start process mesh\n";
        // Data to fill
        std::vector<MeshComponent::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MaterialComponent::Texture> textures;

        // Walk through each of the mesh's vertices
        for (uint32_t i = 0; i < mesh->mNumVertices; i++) 
        {
            MeshComponent::Vertex vertex;

            vertex.pos = convertVec3(mesh->mVertices[i]);

            if(mesh->mNormals)
                vertex.normal = convertVec3(mesh->mNormals[i]);
            else
                vertex.normal = glm::vec3(0);


            if (mesh->mTextureCoords[0])
            {
                vertex.uv.x = (float)mesh->mTextureCoords[0][i].x;
                vertex.uv.y = (float)mesh->mTextureCoords[0][i].y;
            }
            else
            {
                assert(!"texture coord nothing!");
                vertex.uv.x = 0;
                vertex.uv.y = 0;
            }

            vertices.push_back(vertex);
        }

        std::vector<VertexBoneData> vbdata;
        mBoneNum += mesh->mNumBones;
        if(mSkeletal)
        {
            loadBones(node, mesh, vbdata);
            //頂点にボーン情報を付加
            for(uint32_t i = 0; i < vertices.size(); ++i)
            {
                vertices[i].joint = glm::make_vec4(vbdata[i].id);
                vertices[i].weight = glm::make_vec4(vbdata[i].weights);
                //std::cerr << to_string(vertices[i].weight0) << "\n"; 
                // float sum = vertices[i].weight.x + vertices[i].weight.y + vertices[i].weight.z + vertices[i].weight.w; 
                // assert(sum <= 1.01f);
                // assert(vertices[i].joint.x < mBoneNum);
                // assert(vertices[i].joint.y < mBoneNum);
                // assert(vertices[i].joint.z < mBoneNum);
                // assert(vertices[i].joint.w < mBoneNum);
                
            }
        }

        if(mesh->mFaces)
        {
            for(unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for(unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
        }

        std::cerr << "indices\n";

        if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < mScene->mNumMaterials) 
        {
            aiMaterial* material = mScene->mMaterials[mesh->mMaterialIndex];
            std::vector<MaterialComponent::Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        }

        std::cerr << "materials\n";

        {
            Loader::Mesh m;
            m.vertices = vertices;
            m.indices = indices;

            std::cerr << "end process mesh\n";
            
            return m;
        }
    }

    void Loader::loadBones(const aiNode* node, const aiMesh* mesh, std::vector<VertexBoneData>& vbdata_out)
    {
        vbdata_out.resize(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumBones; i++) 
        {
            uint32_t boneIndex = 0;
            std::string boneName(mesh->mBones[i]->mName.data);

            if (mSkeleton.boneMap.count(boneName) <= 0)//そのボーンは登録されてない
            {
                boneIndex = mSkeleton.bones.size();
                mSkeleton.bones.emplace_back();
            }
            else //あった
                boneIndex = mSkeleton.boneMap[boneName];

            mSkeleton.boneMap[boneName] = boneIndex;
            mSkeleton.bones[boneIndex].offset = convert4x4(mesh->mBones[i]->mOffsetMatrix);
            // for(size_t i = 0; i < 3; ++i)
            //     if(abs(mSkeleton.bones[boneIndex].offset[i][3]) > std::numeric_limits<float>::epsilon())
            //     {
            //         std::cerr << boneIndex << "\n";
            //         std::cerr << glm::to_string(mSkeleton.bones[boneIndex].offset) << "\n";
            //         assert(!"invalid bone offset!");
            //     }
            mSkeleton.bones[boneIndex].transform = glm::mat4(1.f);
            //頂点セット
            for (uint j = 0; j < mesh->mBones[i]->mNumWeights; j++) 
            {
                size_t vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
                float weight = mesh->mBones[i]->mWeights[j].mWeight;
                for (uint k = 0; k < 4; k++)
                {
                    if (vbdata_out[vertexID].weights[k] == 0.0) 
                    {
                        vbdata_out[vertexID].id[k]      = boneIndex;
                        vbdata_out[vertexID].weights[k] = weight;
                        break;
                    }

                    // if(k == 3)//まずい
                    //     assert(!"invalid bone weight!");
                }
            }
        }

    }

    //from sample
    std::vector<MaterialComponent::Texture> Loader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
    {
        std::vector<MaterialComponent::Texture> textures;
        for (uint32_t i = 0; i < mat->GetTextureCount(type); i++) 
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            for (uint32_t j = 0; j < mTexturesLoaded.size(); j++) 
            {
                if (std::strcmp(mTexturesLoaded[j].path.c_str(), str.C_Str()) == 0) 
                {
                    textures.push_back(mTexturesLoaded[j]);
                    skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            {   // If texture hasn't been loaded already, load it
                MaterialComponent::Texture texture;

                const aiTexture* embeddedTexture = mScene->GetEmbeddedTexture(str.C_Str());
                if (embeddedTexture != nullptr) 
                {
                    Cutlass::TextureInfo ti;
                    ti.setSRTex2D(embeddedTexture->mWidth, embeddedTexture->mHeight, true);
                    Cutlass::Result res = mContext->createTexture(ti, texture.handle);
                    if (res != Cutlass::Result::eSuccess)
                        assert(!"failed to create embedded texture!");
                    res = mContext->writeTexture(embeddedTexture->pcData, texture.handle);
                    if (res != Cutlass::Result::eSuccess)
                        assert(!"failed to write to embedded texture!");
                } 
                else 
                {
                    std::string filename = std::string(str.C_Str());
                    filename = mDirectory + '/' + filename;
                    std::wstring filenamews = std::wstring(filename.begin(), filename.end());
                    Cutlass::Result res = mContext->createTextureFromFile(filename.c_str(), texture.handle);
                    if (res != Cutlass::Result::eSuccess)
                        assert(!"failed to create material texture!");
                }
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                mTexturesLoaded.emplace_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }

    void Loader::loadMaterialTexture(const char* path, const char* type, std::shared_ptr<MaterialComponent>& material_out)
    {
        material_out->addTexture(loadTexture(path, type));
    }

    void Loader::loadSprite(const char* path, std::shared_ptr<SpriteComponent>& sprite_out)
    {
        SpriteComponent::Sprite sprite;
        
        Cutlass::HTexture handle;
        if(Cutlass::Result::eSuccess != mContext->createTextureFromFile(path, handle))
            assert(!"failed to load sprite texture!");

        sprite.handles.resize(1);
        sprite.handles[0] = handle;
        
        sprite_out->create(sprite);          
    }

    void Loader::loadSprite(std::vector<const char*> pathes, std::shared_ptr<SpriteComponent>& sprite_out)
    {
        SpriteComponent::Sprite sprite;

        sprite.handles.reserve(pathes.size());

        for(const auto& path : pathes)
        {
            Cutlass::HTexture handle;
            if(Cutlass::Result::eSuccess != mContext->createTextureFromFile(path, handle))
                assert(!"failed to load sprite texture!");
            
            sprite.handles.emplace_back(handle);
        }
        
        sprite_out->create(sprite);            
    }

    MaterialComponent::Texture Loader::loadTexture(const char* path, const char* type)
    {
        MaterialComponent::Texture texture;
        if(!path)
        {
            assert(!"invalid texture path(nullptr)!");
            return texture;
        }
        texture.path = std::string(path);
        if(type)
            texture.type = std::string(type);
        else
            texture.type =  texture.path.substr(texture.path.find_last_of("/\\"), texture.path.size());

        if(Cutlass::Result::eSuccess != mContext->createTextureFromFile(path, texture.handle))
        {
            assert(!"failed to load texture!");
        }

        return texture;
    }
    

    // void Loader::load
    // (
    //     const char* path,
    //     std::shared_ptr<MeshComponent>& mesh_out,
    //     std::shared_ptr<MaterialComponent>& material_out
    // )
    // {//omission
    //     load(path);
    //     getMesh(mesh_out);
    //     getMaterial(material_out);
    // }

    // void Loader::loadObj
    // (
    //     const char* path,
    //     std::shared_ptr<MeshComponent>& mesh_out,
    //     std::shared_ptr<MaterialComponent>& material_out
    // )
    // {
    //     ObjModel model;
    //     std::string warn, err;

    //     bool res = tinyobj::LoadObj(&model.attrib, &model.shapes, &model.materials, &warn, &err, path);
        
    //     {
    //         if (!warn.empty()) 
    //             std::cout << "warn: " << warn << std::endl;

    //         if (!err.empty()) 
    //             std::cout << "error: " << err << std::endl;

    //         if (!res)
    //         {
    //             std::cout << "Failed to load Obj: " << path << std::endl;
    //             assert(0);
    //         }
    //         else
    //             std::cout << "Loaded Obj: " << path << std::endl;  
    //     }

    //     //each material
    //     //std::unordered_map<int, std::vector<MeshComponent::Vertex>> verticesMap;
    //     std::unordered_map<MeshComponent::Vertex, uint32_t> uniqueVertices;

    //     std::vector<MeshComponent::Vertex> vertices;
    //     std::vector<uint32_t> indices;

    //     {//load Mesh and Material
    //         MeshComponent::Vertex vertex;

    //         for (const auto& shape : model.shapes)
    //         {
    //             for (const auto& index : shape.mesh.indices) 
    //             {
    //                 int currentMaterialID = shape.mesh.material_ids[index.vertex_index];

    //                 vertex.pos = 
    //                 {
    //                     model.attrib.vertices[3 * index.vertex_index + 0],
    //                     model.attrib.vertices[3 * index.vertex_index + 2],
    //                     model.attrib.vertices[3 * index.vertex_index + 1]
    //                 };

    //                 vertex.uv = 
    //                 {
    //                     model.attrib.texcoords[2 * index.texcoord_index + 0],
    //                     1.0f - model.attrib.texcoords[2 * index.texcoord_index + 1]
    //                 };

    //                 vertex.normal = {1.f, 1.f, 1.f};
    //                 vertex.joint = vertex.weight = glm::vec4(0);
    //                 // {
    //                 //     model.attrib.normals[3 * index.normal_index + 0],
    //                 //     model.attrib.normals[3 * index.normal_index + 1],
    //                 //     model.attrib.normals[3 * index.normal_index + 2]
    //                 // }; 

    //                 if (uniqueVertices.count(vertex) == 0) 
    //                 {
    //                     uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
    //                     vertices.emplace_back(vertex);
    //                 }
    //                 indices.emplace_back(uniqueVertices[vertex]);
                    
    //                 // if(verticesMap.count(currentMaterialID) == 0)
    //                 // {
    //                 //     std::vector<MeshComponent::Vertex> vertices = {vertex};
    //                 //     verticesMap.emplace(currentMaterialID, vertices);
    //                 // }
    //                 // else
    //                 //     verticesMap[currentMaterialID].emplace_back(vertex);
                    
    //                 // indices.emplace_back(index.vertex_index);
    //             }
    //         }

    //         MaterialComponent::PhongMaterialParam param;
    //         bool useTexture = false;
    //         uint32_t useIndexNum = 0;
    //         //現状単一マテリアルしかサポートできない
    //         for(const auto& mtl : model.materials)
    //         {
    //             //auto&& mtl = model.materials[m.first];

    //             param.ambient = real3ToVec4(mtl.ambient);
    //             param.diffuse = real3ToVec4(mtl.diffuse);
    //             param.specular = real3ToVec4(mtl.specular);
    //             param.useTexture = mtl.diffuse_texname.empty() ? glm::uvec1(0) : glm::uvec1(1);

    //             material_out->addMaterialParam<MaterialComponent::PhongMaterialParam, MaterialComponent::MaterialType::ePhong>
    //             (
    //                 param, 
    //                 std::optional(static_cast<std::string_view>(mtl.diffuse_texname)), 
    //                 std::nullopt
    //             );
    //         }
    //     }
    //     //join all vertices
    //     // std::vector<MeshComponent::Vertex> vertices;
    //     // for(const auto& m : verticesMap)
    //     // {
    //     //     vertices.reserve(vertices.size() + m.second.size());
    //     //     std::copy(m.second.begin(), m.second.end(), std::back_inserter(vertices));
    //     // }


    //     {
    //         mModel = model;
    //         mMode = Mode::eObj;

    //         std::cerr << "vertex count : " << vertices.size() << "\n";
    //         std::cerr << "index count : " << indices.size() << "\n";
    //         mesh_out->create(vertices, indices);
    //         mesh_out->setTopology(Cutlass::Topology::eTriangleList);
    //         mesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));
    //     }
    // }

    // void Loader::loadGLTF
    // (
    //     const char* path,
    //     std::shared_ptr<MeshComponent>& mesh_out,
    //     std::shared_ptr<MaterialComponent>& material_out
    // )
    // {
    //     tinygltf::Model model;
    //     {
    //         tinygltf::TinyGLTF loader;
    //         std::string err;
    //         std::string warn;

    //         bool res = loader.LoadASCIIFromFile(&model, &err, &warn, path);

    //         {
    //             if (!warn.empty()) 
    //                 std::cout << "warn: " << warn << std::endl;

    //             if (!err.empty()) 
    //                 std::cout << "error: " << err << std::endl;

    //             if (!res)
    //             {
    //                 std::cout << "Failed to load glTF: " << path << std::endl;
    //                 assert(0);
    //             }
    //             else
    //                 std::cout << "Loaded glTF: " << path << std::endl;
    //         }
    //     }

    //     {
    //         std::vector<MeshComponent::Vertex> vertices;
    //         std::vector<uint32_t> indices;
    //         for(const auto& mesh : model.meshes)
    //         for (size_t j = 0; j < mesh.primitives.size(); j++) 
    //         {
    //             const tinygltf::Primitive &primitive = mesh.primitives[j];
    //             uint32_t indexStart = static_cast<uint32_t>(indices.size());
    //             uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
    //             uint32_t indexCount = 0;
    //             uint32_t vertexCount = 0;
    //             glm::vec3 posMin{};
    //             glm::vec3 posMax{};
    //             bool hasSkin = false;
    //             bool hasIndices = primitive.indices > -1;
    //             // Vertices
    //             {
    //                 const float *bufferPos = nullptr;
    //                 const float *bufferNormals = nullptr;
    //                 const float *bufferTexCoordSet0 = nullptr;
    //                 const float *bufferTexCoordSet1 = nullptr;
    //                 const uint16_t *bufferJoints = nullptr;
    //                 const float *bufferWeights = nullptr;

    //                 int posByteStride;
    //                 int normByteStride;
    //                 int uv0ByteStride;
    //                 int uv1ByteStride;
    //                 int jointByteStride;
    //                 int weightByteStride;

    //                 // Position attribute is required
    //                 assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

    //                 const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
    //                 const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
    //                 bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
    //                 posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
    //                 posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
    //                 vertexCount = static_cast<uint32_t>(posAccessor.count);
    //                 posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);

    //                 if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) 
    //                 {
    //                     const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
    //                     const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
    //                     bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
    //                     normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);
    //                 }

    //                 if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) 
    //                 {
    //                     const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
    //                     const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
    //                     bufferTexCoordSet0 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
    //                     uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
    //                 }

    //                 if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) 
    //                 {
    //                     const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
    //                     const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
    //                     bufferTexCoordSet1 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
    //                     uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
    //                 }

    //                 // Skinning
    //                 // Joints
    //                 if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) 
    //                 {
    //                     const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
    //                     const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
    //                     bufferJoints = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
    //                     jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / sizeof(bufferJoints[0])) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
    //                 }

    //                 if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) 
    //                 {
    //                     const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
    //                     const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];
    //                     bufferWeights = reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
    //                     weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
    //                 }

    //                 hasSkin = (bufferJoints && bufferWeights);

    //                 for (size_t v = 0; v < posAccessor.count; v++) 
    //                 {
    //                     MeshComponent::Vertex vert;
    //                     vert.pos = glm::make_vec3(&bufferPos[v * posByteStride]);//glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
    //                     vert.normal = -glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
    //                     //vert.color = glm::vec4(1.f);
    //                     vert.uv = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
    //                     vert.joint = vert.weight = glm::vec4(0);
    //                     // vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
    //                     // vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
                        
    //                     // vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * jointByteStride])) : glm::vec4(0.0f);
    //                     // vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
    //                     //Fix for all zero weights
    //                     // if (glm::length(vert.weight0) == 0.0f) 
    //                     // {
    //                     //     vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    //                     // }

    //                     vertices.emplace_back(vert);
    //                 }
    //             }
    //             // Indices
    //             if (hasIndices)
    //             {
    //                 const tinygltf::Accessor &accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
    //                 const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
    //                 const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

    //                 indexCount = static_cast<uint32_t>(accessor.count);
    //                 const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

    //                 switch (accessor.componentType) 
    //                 {
    //                 case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
    //                 {
    //                     const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
    //                     for (size_t index = 0; index < accessor.count; index++) 
    //                     {
    //                         indices.emplace_back(buf[index] + vertexStart);
    //                     }
    //                     break;
    //                 }
    //                 //uint32_tにしか対応してません(TODO)
    //                 case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
    //                 {
    //                     const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
    //                     for (size_t index = 0; index < accessor.count; index++) 
    //                     {
    //                         indices.emplace_back(static_cast<uint32_t>(buf[index] + vertexStart));
    //                     }
    //                     break;
    //                 }
    //                 case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
    //                 {
    //                     const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
    //                     for (size_t index = 0; index < accessor.count; index++) 
    //                     {
    //                         indices.emplace_back(static_cast<uint32_t>(buf[index] + vertexStart));
    //                     }
    //                     break;
    //                 }
    //                 default:
    //                     std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
    //                     return;
    //                 }

    //             }					
    //         }
    //         mesh_out->create(vertices, indices);
    //         mesh_out->setTopology(Cutlass::Topology::eTriangleList);
    //         mesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eNone, Cutlass::FrontFace::eClockwise));
    //     }
        
    //     mModel = model;
    //     mMode = Mode::eGLTF;
    // }

    // void Loader::getMesh(std::shared_ptr<MeshComponent>& mesh_out)
    // {
    //     std::vector<uint32_t> indices;

    //     switch(mMode)
    //     {
    //     case Mode::eObj:
    //         {
    //             auto&& model = std::get<ObjModel>(mModel);
    //             //each material
    //             std::map<int, std::vector<MeshComponent::Vertex>> vertices;
    //             MeshComponent::Vertex vertex;
    //             std::unordered_map<MeshComponent::Vertex, uint32_t> uniqueVertices{};

    //             for (const auto& shape : model.shapes)
    //             {
    //                 for (const auto& index : shape.mesh.indices) 
    //                 {
    //                     vertex.pos = 
    //                     {
    //                         model.attrib.vertices[3 * index.vertex_index + 0],
    //                         model.attrib.vertices[3 * index.vertex_index + 1],
    //                         model.attrib.vertices[3 * index.vertex_index + 2]
    //                     };

    //                     vertex.UV = 
    //                     {
    //                         model.attrib.texcoords[2 * index.texcoord_index + 0],
    //                         1.0f - model.attrib.texcoords[2 * index.texcoord_index + 1]
    //                     };

    //                     vertex.color = {1.0f, 1.0f, 1.0f, 1.f};
    //                     vertex.normal = glm::vec3(model.attrib.normals[0], model.attrib.normals[1], model.attrib.normals[2]);

    //                     // if (uniqueVertices.count(vertex) == 0) 
    //                     // {
    //                     //     uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
    //                     //     vertices.emplace_back(vertex);
    //                     // }
    //                     vertices.emplace_back(shape.mesh.material_ids, )

    //                     indices.emplace_back(uniqueVertices[vertex]);
    //                 }
    //             }
    //             std::cerr << "vertex count : " << vertices.size() << "\n";
    //             std::cerr << "index count : " << indices.size() << "\n";
    //             mesh_out->create<MeshComponent::Vertex>(vertices, indices);
    //             mesh_out->setTopology(Cutlass::Topology::eTriangleList);
    //             mesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));
    //         }
    //         break;
    //     case Mode::eGLTF:
    //         {
    //             std::vector<MeshComponent::GLTFVertex> vertices;
    //             auto&& model = std::get<tinygltf::Model>(mModel);
    //             for(const auto& mesh : model.meshes)
    //             for (size_t j = 0; j < mesh.primitives.size(); j++) 
    //             {
    //                 const tinygltf::Primitive &primitive = mesh.primitives[j];
    //                 uint32_t indexStart = static_cast<uint32_t>(indices.size());
    //                 uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
    //                 uint32_t indexCount = 0;
    //                 uint32_t vertexCount = 0;
    //                 glm::vec3 posMin{};
    //                 glm::vec3 posMax{};
    //                 bool hasSkin = false;
    //                 bool hasIndices = primitive.indices > -1;
    //                 // Vertices
    //                 {
    //                     const float *bufferPos = nullptr;
    //                     const float *bufferNormals = nullptr;
    //                     const float *bufferTexCoordSet0 = nullptr;
    //                     const float *bufferTexCoordSet1 = nullptr;
    //                     const uint16_t *bufferJoints = nullptr;
    //                     const float *bufferWeights = nullptr;

    //                     int posByteStride;
    //                     int normByteStride;
    //                     int uv0ByteStride;
    //                     int uv1ByteStride;
    //                     int jointByteStride;
    //                     int weightByteStride;

    //                     // Position attribute is required
    //                     assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

    //                     const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
    //                     const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
    //                     bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
    //                     posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
    //                     posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
    //                     vertexCount = static_cast<uint32_t>(posAccessor.count);
    //                     posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);

    //                     if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) 
    //                     {
    //                         const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
    //                         const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
    //                         bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
    //                         normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);
    //                     }

    //                     if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) 
    //                     {
    //                         const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
    //                         const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
    //                         bufferTexCoordSet0 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
    //                         uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
    //                     }

    //                     if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) 
    //                     {
    //                         const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
    //                         const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
    //                         bufferTexCoordSet1 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
    //                         uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
    //                     }

    //                     // Skinning
    //                     // Joints
    //                     if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) 
    //                     {
    //                         const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
    //                         const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
    //                         bufferJoints = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
    //                         jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / sizeof(bufferJoints[0])) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
    //                     }

    //                     if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) 
    //                     {
    //                         const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
    //                         const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];
    //                         bufferWeights = reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
    //                         weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
    //                     }

    //                     hasSkin = (bufferJoints && bufferWeights);

    //                     for (size_t v = 0; v < posAccessor.count; v++) 
    //                     {
    //                         MeshComponent::GLTFVertex vert;
    //                         vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
    //                         vert.normal = -glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
    //                         vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
    //                         vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
                            
    //                         vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * jointByteStride])) : glm::vec4(0.0f);
    //                         vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
    //                         //Fix for all zero weights
    //                         if (glm::length(vert.weight0) == 0.0f) 
    //                         {
    //                             vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	// 					    }

    //                         vertices.emplace_back(vert);
	// 				    }
	// 			    }
    //                 // Indices
    //                 if (hasIndices)
    //                 {
    //                     const tinygltf::Accessor &accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
    //                     const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
    //                     const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

    //                     indexCount = static_cast<uint32_t>(accessor.count);
    //                     const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

    //                     switch (accessor.componentType) 
    //                     {
    //                     case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
    //                     {
    //                         const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
    //                         for (size_t index = 0; index < accessor.count; index++) 
    //                         {
    //                             indices.emplace_back(buf[index] + vertexStart);
    //                         }
    //                         break;
    //                     }
    //                     //uint32_tにしか対応してません(TODO)
    //                     case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
    //                     {
    //                         const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
    //                         for (size_t index = 0; index < accessor.count; index++) 
    //                         {
    //                             indices.emplace_back(static_cast<uint32_t>(buf[index] + vertexStart));
    //                         }
    //                         break;
    //                     }
    //                     case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
    //                     {
    //                         const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
    //                         for (size_t index = 0; index < accessor.count; index++) 
    //                         {
    //                             indices.emplace_back(static_cast<uint32_t>(buf[index] + vertexStart));
    //                         }
    //                         break;
    //                     }
    //                     default:
    //                         std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
    //                         return;
    //                     }
    //                 }					
    //             }
    //             mesh_out->create<MeshComponent::GLTFVertex>(vertices, indices);
    //             mesh_out->setTopology(Cutlass::Topology::eTriangleList);
    //             mesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eNone, Cutlass::FrontFace::eClockwise));
    //         }
    //         break;
    //     default:
    //         assert(!"model isn't loaded!");
    //         break;
    //     }

    // }

    // inline auto real3ToVec4(const tinyobj::real_t src[], const float w = 1.f) -> glm::vec4
    // {
    //     return glm::vec4(src[0], src[1], src[2], w);
    // };

    // inline auto real4ToVec4(const tinyobj::real_t src[]) -> glm::vec4
    // {
    //     return glm::vec4(src[0], src[1], src[2], src[4]);
    // };


    // void Loader::getMaterial(std::shared_ptr<MaterialComponent>& material_out)
    // {
    //     switch(mMode)
    //     {
    //     case Mode::eObj:
    //         {
    //             auto&& model = std::get<ObjModel>(mModel);

    //             MaterialComponent::PhongMaterialParam param;
    //             bool useTexture = false;
    //             uint32_t useVertexNum = 0;
    //             for(const auto& mtl : model.materials)
    //             {
    //                 param.ambient = real3ToVec4(mtl.ambient);
    //                 param.diffuse = real3ToVec4(mtl.diffuse);
    //                 param.specular = real3ToVec4(mtl.specular);
    //                 param.edgeFlag = glm::uvec1(1);
    //                 param.useTexture = mtl.diffuse_texname.empty() ? glm::uvec1(0) : glm::uvec1(1);

    //                 material_out->addMaterialParam<MaterialComponent::PhongMaterialParam>(param, std::optional(static_cast<std::string_view>(mtl.diffuse_texname)), );
    //             }
    //         }
    //         break;
    //     case Mode::eGLTF:
    //         {
    //             auto&& model = std::get<tinygltf::Model>(mModel);

    //         }
    //         break;
    //     default:
    //         assert(!"model isn't loaded!");
    //         break;
    //     }
    // }
}