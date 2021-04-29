#include <Engine/System/Loader.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define TINYOBJLOADER_IMPLEMENTATION
#include <Engine/ThirdParty/tiny_obj_loader.h>
#include <Engine/ThirdParty/tiny_gltf.h>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/MaterialComponent.hpp>

#include <iostream>
#include <unordered_map>

#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

//ユニークな頂点を抽出するため,頂点型について書き換える必要がある
namespace std 
{
    template<> struct hash<Engine::MeshComponent::Vertex>
    {
        size_t operator()(Engine::MeshComponent::Vertex const& vertex) const 
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.UV) << 1);
        }
    };
}

namespace Engine
{
    inline bool ends_with(const std::string& str, const std::string& suffix) 
    {
        size_t len1 = str.size();
        size_t len2 = suffix.size();
        return len1 >= len2 && str.compare(len1 - len2, len2, suffix) == 0;
    }

    void Loader::load(const char* _path)
    {
        std::string path(_path);

        if(ends_with(path, ".obj"))
        {
            ObjModel model;
            std::string warn, err;

            bool res = tinyobj::LoadObj(&model.attrib, &model.shapes, &model.materials, &warn, &err, path.c_str());
            
            {
                if (!warn.empty()) 
                    std::cout << "warn: " << warn << std::endl;

                if (!err.empty()) 
                    std::cout << "error: " << err << std::endl;

                if (!res)
                {
                    std::cout << "Failed to load Obj: " << path << std::endl;
                    assert(0);
                }
                else
                    std::cout << "Loaded Obj: " << path << std::endl;  
            }  
            
            mModel = model;
            mMode = Mode::eObj;
        }
        else if(ends_with(path, ".gltf") || ends_with(path, ".vrm"))
        {
            tinygltf::Model model;
            
            tinygltf::TinyGLTF loader;
            std::string err;
            std::string warn;

            bool res = loader.LoadASCIIFromFile(&model, &err, &warn, path);

            {
                if (!warn.empty()) 
                    std::cout << "warn: " << warn << std::endl;

                if (!err.empty()) 
                    std::cout << "error: " << err << std::endl;

                if (!res)
                {
                    std::cout << "Failed to load glTF: " << path << std::endl;
                    assert(0);
                }
                else
                    std::cout << "Loaded glTF: " << path << std::endl;
            }

            mModel = model;
            mMode = Mode::eGLTF;
        }
        else
        {
            assert(!"This format is not supported!");
            mMode = Mode::eNone;
        }
    }

    void Loader::load
    (
        const char* path,
        std::shared_ptr<MeshComponent>& mesh_out,
        std::shared_ptr<MaterialComponent>& material_out
    )
    {//手抜き
        load(path);
        getMesh(mesh_out);
        getMaterial(material_out);
    }

    void Loader::getMesh(std::shared_ptr<MeshComponent>& mesh_out)
    {
        std::vector<uint32_t> indices;

        switch(mMode)
        {
        case Mode::eObj:
            {
                auto&& model = std::get<ObjModel>(mModel);
                std::vector<MeshComponent::Vertex> vertices;
                MeshComponent::Vertex vertex;
                std::unordered_map<MeshComponent::Vertex, uint32_t> uniqueVertices{};

                for (const auto& shape : model.shapes) 
                    for (const auto& index : shape.mesh.indices) 
                    {
                        vertex.pos = 
                        {
                            model.attrib.vertices[3 * index.vertex_index + 0],
                            model.attrib.vertices[3 * index.vertex_index + 1],
                            model.attrib.vertices[3 * index.vertex_index + 2]
                        };

                        vertex.UV = 
                        {
                            model.attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - model.attrib.texcoords[2 * index.texcoord_index + 1]
                        };

                        vertex.color = {1.0f, 1.0f, 1.0f, 1.f};
                        vertex.normal = {0, 1.f, 0};

                        if (uniqueVertices.count(vertex) == 0) 
                        {
                            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                            vertices.emplace_back(vertex);
                        }

                        indices.emplace_back(uniqueVertices[vertex]);
                    }
                std::cerr << "vertex count : " << vertices.size() << "\n";
                std::cerr << "index count : " << indices.size() << "\n";
                mesh_out->create<MeshComponent::Vertex>(vertices, indices);
                mesh_out->setTopology(Cutlass::Topology::eTriangleList);
                mesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));
            }
            break;
        case Mode::eGLTF:
            {
                std::vector<MeshComponent::GLTFVertex> vertices;
                auto&& model = std::get<tinygltf::Model>(mModel);
                for(const auto& mesh : model.meshes)
                for (size_t j = 0; j < mesh.primitives.size(); j++) 
                {
                    const tinygltf::Primitive &primitive = mesh.primitives[j];
                    uint32_t indexStart = static_cast<uint32_t>(indices.size());
                    uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
                    uint32_t indexCount = 0;
                    uint32_t vertexCount = 0;
                    glm::vec3 posMin{};
                    glm::vec3 posMax{};
                    bool hasSkin = false;
                    bool hasIndices = primitive.indices > -1;
                    // Vertices
                    {
                        const float *bufferPos = nullptr;
                        const float *bufferNormals = nullptr;
                        const float *bufferTexCoordSet0 = nullptr;
                        const float *bufferTexCoordSet1 = nullptr;
                        const uint16_t *bufferJoints = nullptr;
                        const float *bufferWeights = nullptr;

                        int posByteStride;
                        int normByteStride;
                        int uv0ByteStride;
                        int uv1ByteStride;
                        int jointByteStride;
                        int weightByteStride;

                        // Position attribute is required
                        assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                        const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
                        bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
                        posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                        posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
                        vertexCount = static_cast<uint32_t>(posAccessor.count);
                        posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);

                        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) 
                        {
                            const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                            const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
                            bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                            normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);
                        }

                        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) 
                        {
                            const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                            const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                            bufferTexCoordSet0 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                            uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
                        }

                        if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) 
                        {
                            const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                            const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                            bufferTexCoordSet1 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                            uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
                        }

                        // Skinning
                        // Joints
                        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) 
                        {
                            const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
                            const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
                            bufferJoints = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
                            jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / sizeof(bufferJoints[0])) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
                        }

                        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) 
                        {
                            const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                            const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];
                            bufferWeights = reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
                            weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
                        }

                        hasSkin = (bufferJoints && bufferWeights);

                        for (size_t v = 0; v < posAccessor.count; v++) 
                        {
                            MeshComponent::GLTFVertex vert;
                            vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                            vert.normal = -glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                            vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
                            vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
                            
                            vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * jointByteStride])) : glm::vec4(0.0f);
                            vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
                            //Fix for all zero weights
                            if (glm::length(vert.weight0) == 0.0f) 
                            {
                                vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
						    }

                            vertices.emplace_back(vert);
					    }
				    }
                    // Indices
                    if (hasIndices)
                    {
                        const tinygltf::Accessor &accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

                        indexCount = static_cast<uint32_t>(accessor.count);
                        const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                        switch (accessor.componentType) 
                        {
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
                        {
                            const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++) 
                            {
                                indices.emplace_back(buf[index] + vertexStart);
                            }
                            break;
                        }
                        //uint32_tにしか対応してません(TODO)
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
                        {
                            const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++) 
                            {
                                indices.emplace_back(static_cast<uint32_t>(buf[index] + vertexStart));
                            }
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
                        {
                            const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++) 
                            {
                                indices.emplace_back(static_cast<uint32_t>(buf[index] + vertexStart));
                            }
                            break;
                        }
                        default:
                            std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                            return;
                        }
                    }					
                }
                mesh_out->create<MeshComponent::GLTFVertex>(vertices, indices);
                mesh_out->setTopology(Cutlass::Topology::eTriangleList);
                mesh_out->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eNone, Cutlass::FrontFace::eClockwise));
            }
            break;
        default:
            assert(!"model isn't loaded!");
            break;
        }

    }

    void Loader::getMaterial(std::shared_ptr<MaterialComponent>& material_out)
    {
        switch(mMode)
        {
        case Mode::eObj:
            {
                
            }
            break;
        case Mode::eGLTF:
            {
                
            }
            break;
        default:
            assert(!"model isn't loaded!");
            break;
        }
    }

    void Loader::unload()
    {
        mMode = Mode::eNone;
    }
}