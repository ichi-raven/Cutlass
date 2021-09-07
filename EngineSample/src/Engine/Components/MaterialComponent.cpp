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
        
    }

    void MaterialComponent::addTexture(const Texture& texture)
    {
        mTextures.emplace_back(texture);
    }

    void MaterialComponent::addTextures(const std::vector<Texture>& textures)
    {
        mTextures.reserve(textures.size());
        std::copy(textures.begin(), textures.end(), std::back_inserter(mTextures));
    }

    void MaterialComponent::clearTextures()
    {
        //ハンドル解放できないのでLoaderに任せる
        mTextures.clear();
    }

    const MaterialComponent::Texture& MaterialComponent::findTexture(std::string_view name) const
    {
        return *std::find_if(mTextures.begin(), mTextures.end(), [&](const Texture& t){return !name.compare(t.type);});
    }

    const std::vector<MaterialComponent::Texture>& MaterialComponent::getTextures() const
    {
        return mTextures;
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

    // const std::vector<MaterialComponent::MaterialSet>& MaterialComponent::getMaterialSets() const
    // {
    //     return mMaterialSets;
    // }

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