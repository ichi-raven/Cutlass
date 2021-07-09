#include <Engine/Components/CustomMaterialComponent.hpp>

#include <Engine/Resources/Resources.hpp>

namespace Engine
{
    CustomMaterialComponent::CustomMaterialComponent()
    {
        setUpdateFlag(false);

        mColorBlend = Cutlass::ColorBlend::eDefault;
        mMultiSampleState = Cutlass::MultiSampleState::eDefault;

        mVS = Cutlass::Shader(Resource::Shader::genPath(Resource::Shader::objVert), "main");
	    mFS = Cutlass::Shader(Resource::Shader::genPath(Resource::Shader::frag), "main");
    }

    CustomMaterialComponent::~CustomMaterialComponent()
    {
		
    }

    // void CustomMaterialComponent::addMaterialParam(const PhongMaterialParam& material, std::optional<std::string_view> texturePath, std::optional<uint32_t> useVertexNum, Cutlass::Context& context)
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

    //const std::vector<CustomMaterialComponent::MaterialSet>& CustomMaterialComponent::getMaterialSets() const
    // {
    //     return mMaterialSets;
    // }

    void CustomMaterialComponent::setVS(const Cutlass::Shader& vertexShader)
    {
        mVS = vertexShader;
    }

    const Cutlass::Shader& CustomMaterialComponent::getVS() const
    {
        return mVS;
    }

    void CustomMaterialComponent::setFS(const Cutlass::Shader& fragmentShader)
    {
        mFS = fragmentShader;
    }

    const Cutlass::Shader& CustomMaterialComponent::getFS() const
    {
        return mFS;
    }

    void CustomMaterialComponent::setColorBlend(Cutlass::ColorBlend colorBlend)
    {
        mColorBlend = colorBlend;
    }

    Cutlass::ColorBlend CustomMaterialComponent::getColorBlend() const
    {
        return mColorBlend;
    }

    void CustomMaterialComponent::setMultiSampleState(Cutlass::MultiSampleState multiSampleState)
    {
        mMultiSampleState = multiSampleState;
    }

    Cutlass::MultiSampleState CustomMaterialComponent::getMultiSampleState() const
    {
        return mMultiSampleState;
    }

    void CustomMaterialComponent::update()
    {
        //do nothing
    }

};