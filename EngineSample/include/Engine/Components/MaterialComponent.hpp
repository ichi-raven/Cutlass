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

        template<typename MaterialParamType, MaterialType materialType>
        void addMaterialParam(const MaterialParamType& material, std::optional<std::string_view> texturePath, std::optional<uint32_t> useIndexNum)
        {
            // auto&& tmp = mMaterialSets.emplace_back();
            // auto&& context = getContext();

            // tmp.useIndexNum = useIndexNum;
            // tmp.type = materialType;

            // Cutlass::HBuffer hBuffer;
            // {
            //     if(Cutlass::Result::eSuccess != context->createBuffer(Cutlass::BufferInfo(sizeof(MaterialParamType), Cutlass::BufferUsage::eUniform, true), hBuffer))
            //         assert(!"Failed to create material param buffer!");
            //     if(Cutlass::Result::eSuccess != context->writeBuffer(sizeof(MaterialParamType), &material, hBuffer))
            //         assert(!"Faield to write material buffer!");
            //     tmp.paramBuffer = hBuffer;
            // }

            // if(texturePath)
            // {
            //     Cutlass::HTexture htex;
            //     if(Cutlass::Result::eSuccess != context->createTextureFromFile(texturePath.value().data(), htex))
            //         assert(!"Failed to create material texture!");
            //     tmp.texture = htex;
            // }
        }

        const std::vector<MaterialSet>& getMaterialSets() const;

        // void setVS(const Cutlass::Shader& shader);
        // const Cutlass::Shader& getVS() const;

        // void setFS(const Cutlass::Shader& shader); 
        // const Cutlass::Shader& getFS() const;

        // void setColorBlend(Cutlass::ColorBlend colorBlend);
        // Cutlass::ColorBlend getColorBlend() const;

        // void setMultiSampleState(Cutlass::MultiSampleState multiSampleState);
        // Cutlass::MultiSampleState getMultiSampleState() const;

        virtual void update() override;

    protected:

        std::vector<MaterialSet> mMaterialSets;

        // Cutlass::Shader mVS;
        // Cutlass::Shader mFS;
        // Cutlass::ColorBlend mColorBlend;
        // Cutlass::MultiSampleState mMultiSampleState;
    };
}