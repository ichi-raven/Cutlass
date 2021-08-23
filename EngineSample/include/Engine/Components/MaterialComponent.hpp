#pragma once
#include "IComponent.hpp"

#include <Cutlass.hpp>

#include <glm/glm.hpp>

namespace Engine
{
    class MaterialComponent : public IComponent
    {
    public:
        struct Texture
        {
            Cutlass::HTexture handle;
            std::string type;
            std::string path;
        };

        //フォンシェーディングする時のマテリアル型
        struct PhongMaterialParam
        {
            glm::vec4 ambient;
            glm::vec4 diffuse;
            glm::vec4 specular;
            glm::uvec1 useTexture;
        };

        struct SimpleMaterialParam
        {
            glm::vec4 color;
        };

        enum class MaterialType
        {
            ePhong,
            ePBR,
            eSimple
        };

        //ビルド済みマテリアルデータ
        struct MaterialSet
        {
            std::optional<uint32_t> useIndexNum;
            std::optional<Cutlass::HTexture> texture;
            Cutlass::HBuffer paramBuffer;
            MaterialType type;
        };

        MaterialComponent();
        virtual ~MaterialComponent() override;

        void addTexture(const Texture& texture);
        void addTextures(const std::vector<Texture>& textures);

        void clearTextures();

        const Texture& findTexture(std::string_view name) const;

        const std::vector<Texture>& getTextures() const;

        virtual void update() override;

    protected:

        std::vector<Texture> mTextures;
        //std::vector<MaterialSet> mMaterialSets;

        // Cutlass::Shader mVS;
        // Cutlass::Shader mFS;
        // Cutlass::ColorBlend mColorBlend;
        // Cutlass::MultiSampleState mMultiSampleState;
    };
}