#pragma once
#include "IComponent.hpp"

#include <Cutlass.hpp>

#include <glm/glm.hpp>

namespace Engine
{
    class MaterialComponent : public IComponent
    {
    public:
        //フォンシェーディングする時のマテリアル型
        struct PhongMaterial
        {
            glm::vec4 diffuse;
            glm::vec4 ambient;
            glm::vec4 specular;
            glm::uvec1 useTexture;
            glm::uvec1 edgeFlag;
        };

        MaterialComponent();

        void addMaterial(const PhongMaterial& material, std::optional<const char*> texturePath, std::optional<uint32_t> useVertexNum);

        void setFS(const Cutlass::Shader& shader);

        // const Cutlass::HBuffer& getMaterialCB();

        virtual void update() override;

    private:
        template<typename MaterialType>
        struct Material
        {
            MaterialType materialType;
            std::optional<Cutlass::HTexture> texture;
            std::optional<uint32_t> useVertexNum;
            std::vector<Cutlass::HBuffer> CBs;
            
        };

        //単一のMeshに対して複数のMaterialがある場合があります
        std::vector<Material<PhongMaterial>> mPhongMaterials;
    };
}