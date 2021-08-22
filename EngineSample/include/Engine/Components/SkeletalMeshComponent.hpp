#pragma once

#include <Cutlass.hpp>
#include <glm/glm.hpp>

#include <typeinfo>

#include "MeshComponent.hpp"

#include "../Utility/Transform.hpp"


namespace Engine
{
    class SkeletalMeshComponent : public MeshComponent
    {
    public:


        SkeletalMeshComponent();
        virtual ~SkeletalMeshComponent();

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