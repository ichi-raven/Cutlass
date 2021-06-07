//Resources
#pragma once
#include <string>

namespace Engine
{
    namespace Resource
    {
        namespace Shader
        {
            constexpr const char* shaderDir = "../resources/shaders/";
            
            constexpr const char* objVert = "MeshWithMaterial/vert.spv";
            constexpr const char* gltfVert = "MeshWithMaterial/gltfvert.spv";

            constexpr const char* frag = "MeshWithMaterial/frag.spv";

            const std::string genPath(const char* target);
        };

        namespace Model
        {
            constexpr const char* modelDir = "../resources/models/";
            constexpr const char* testObj = "chalet.obj";
            constexpr const char* testGLTF = "Cube.gltf";

            const std::string genPath(const char* target);
        };
    }
}

