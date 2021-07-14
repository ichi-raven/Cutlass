#include <Engine/Components/MaterialComponent.hpp>

#include <Engine/Resources/Resources.hpp>

namespace Engine
{
    MaterialComponent::MaterialComponent()
    {
        setUpdateFlag(false);
    }

    MaterialComponent::~MaterialComponent()
    {
        auto&& context = getContext();
        for(const auto& material : mMaterialSets)
        {
            context->destroyBuffer(material.paramBuffer);
            if(material.texture)
                context->destroyTexture(material.texture.value());
        }
    }

    // void MaterialComponent::addMaterialParam(const PhongMaterialParam& material, std::optional<std::string_view> texturePath, std::optional<uint32_t> useVertexNum, Cutlass::Context& context)
    // {
    //     auto& tmp = mPhongMaterials.emplace_back(material);

    //     if(texturePath)
    //     {
    //         Cutlass::HTexture htex;
    //         context.createTextureFromFile(texturePath.value().data(), htex);
    //         mPhongMaterials.emplace_back(material).texture = htex; 
    //     }
        
    //     Cutlass::HBuffer hBuffer;
    //     context.createBuffer(Cutlass::BufferInfo(sizeof(PhongMaterialParam), ))

    // }

    const std::vector<MaterialComponent::MaterialSet>& MaterialComponent::getMaterialSets() const
    {
        return mMaterialSets;
    }

    // void MaterialComponent::setVS(const Cutlass::Shader& vertexShader)
    // {
    //     mVS = vertexShader;
    // }

    // const Cutlass::Shader& MaterialComponent::getVS() const
    // {
    //     return mVS;
    // }

    // void MaterialComponent::setFS(const Cutlass::Shader& fragmentShader)
    // {
    //     mFS = fragmentShader;
    // }

    // const Cutlass::Shader& MaterialComponent::getFS() const
    // {
    //     return mFS;
    // }

    // void MaterialComponent::setColorBlend(Cutlass::ColorBlend colorBlend)
    // {
    //     mColorBlend = colorBlend;
    // }

    // Cutlass::ColorBlend MaterialComponent::getColorBlend() const
    // {
    //     return mColorBlend;
    // }

    // void MaterialComponent::setMultiSampleState(Cutlass::MultiSampleState multiSampleState)
    // {
    //     mMultiSampleState = multiSampleState;
    // }

    // Cutlass::MultiSampleState MaterialComponent::getMultiSampleState() const
    // {
    //     return mMultiSampleState;
    // }

    void MaterialComponent::update()
    {
        //do nothing
    }

};