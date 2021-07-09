#pragma once

#include <Cutlass.hpp>
#include <glm/glm.hpp>

#include <typeinfo>

#include "MeshComponent.hpp"

#include "../Utility/Transform.hpp"


namespace Engine
{
    class SkeltalMeshComponent : public MeshComponent
    {
    public:
        //頂点構造, これをいじるとRendererとかも変わります
        #define EQ(MEMBER) (MEMBER == another.MEMBER)//便利
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec4 color;
            glm::vec3 normal;
            glm::vec2 UV;

            bool operator==(const Vertex& another) const 
            {
                return EQ(pos) && EQ(color) && EQ(normal) && EQ(UV);
            }
        };

        struct CPUVertex
        {
            glm::vec3 pos;

            bool operator==(const CPUVertex& another) const 
            {
                return EQ(pos);
            }
        };

        SkeltalMeshComponent();
        virtual ~SkeltalMeshComponent();

        //メッシュを構築する
        void create
        (
            const std::vector<Vertex>& vertices,
            const std::vector<uint32_t>& indices
        );

        virtual void update() override;

    private:
		
    };
};