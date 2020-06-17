#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <vector>

#include "BufferObject.hpp"

namespace Cutlass
{
    //ここに頂点の構造体を記述すると楽かも知れない
//#include "VertexTypes.hpp"

    class Device;

    class Mesh
    {
    public:
        enum class VertexType
        {
            eDefault,
        };

        Mesh();
        ~Mesh();

        void create();

        void assign(Device *pDevice);

        void destroy();

    private:
        struct DefaultVertex
        {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec3 uv;
        };

        BufferObject mVertexBuffer;
        BufferObject mIndexBuffer;
    };
};