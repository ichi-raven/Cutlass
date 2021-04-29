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
            glm::vec4 diffuse;
            glm::vec4 ambient;
            glm::vec4 specular;
            glm::uvec1 useTexture;
            glm::uvec1 edgeFlag;
        };

        //ビルド済みマテリアルデータ
        struct MaterialSet
        {
            std::optional<uint32_t> useVertexNum;
            std::optional<Cutlass::HTexture> texture;
            Cutlass::HBuffer paramBuffer;
        };

        MaterialComponent();
        virtual ~MaterialComponent();

        template<typename MaterialParamType>
        void addMaterialParam(const MaterialParamType& material, std::optional<std::string_view> texturePath, std::optional<uint32_t> useVertexNum)
        {
            auto& tmp = mMaterialSets.emplace_back(material);

            tmp.useVertexNum = useVertexNum;

            Cutlass::HBuffer hBuffer;
            if(Cutlass::Result::eSuccess != mContext->createBuffer(Cutlass::BufferInfo(sizeof(MaterialParamType), Cutlass::BufferUsage::eUniform, true), hBuffer))
                assert(!"Failed to create material param buffer!");
            tmp.paramBuffer = hBuffer;

            if(texturePath)
            {
                Cutlass::HTexture htex;
                if(Cutlass::Result::eSuccess != mContext->createTextureFromFile(texturePath.value().data(), htex))
                    assert(!"Failed to create material texture!");
                tmp.texture = htex; 
            }
        }

        const std::vector<MaterialSet>& getMaterialSets() const;

        void setVS(const Cutlass::Shader& shader);
        const Cutlass::Shader& getVS() const;

        void setFS(const Cutlass::Shader& shader); 
        const Cutlass::Shader& getFS() const;

        void setColorBlend(Cutlass::ColorBlend colorBlend);
        Cutlass::ColorBlend getColorBlend() const;

        void setMultiSampleState(Cutlass::MultiSampleState multiSampleState);
        Cutlass::MultiSampleState getMultiSampleState() const;

        virtual void update() override;

    private:

        std::vector<MaterialSet> mMaterialSets;

        Cutlass::Shader mVS;
        Cutlass::Shader mFS;
        Cutlass::ColorBlend mColorBlend;
        Cutlass::MultiSampleState mMultiSampleState;
    };
}