#include <Engine/Components/MaterialComponent.hpp>

namespace Engine
{
    MaterialComponent::MaterialComponent()
    {
        //一応アップデートは不要なものとしておく
        setUpdateFlag();
        mColorBlend = Cutlass::ColorBlend::eDefault;
        mTopology = Cutlass::Topology::eTriangleList;
        mRasterizerState = Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eClockwise);
        mMultiSampleState = Cutlass::MultiSampleState::eDefault;

        mVS = Cutlass::Shader("../resources/shaders/MeshWithMaterial/vert.spv", "main");
	    mFS = Cutlass::Shader("../resources/shaders/MeshWithMaterial/frag.spv", "main");
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

    void MaterialComponent::setVS(const Cutlass::Shader& vertexShader)
    {
        mVS = vertexShader;
    }

    const Cutlass::Shader& MaterialComponent::getVS() const
    {
        return mVS;
    }

    void MaterialComponent::setFS(const Cutlass::Shader& fragmentShader)
    {
        mFS = fragmentShader;
    }

    const Cutlass::Shader& MaterialComponent::getFS() const
    {
        return mFS;
    }

    void MaterialComponent::setColorBlend(Cutlass::ColorBlend colorBlend)
    {
        mColorBlend = colorBlend;
    }

    Cutlass::ColorBlend MaterialComponent::getColorBlend() const
    {
        return mColorBlend;
    }

    void MaterialComponent::setTopology(Cutlass::Topology topology)
    {
        mTopology = topology;
    }

    Cutlass::Topology MaterialComponent::getTopology() const
    {
        return mTopology;
    }
    
    void MaterialComponent::setRasterizerState(const Cutlass::RasterizerState& rasterizerState)
    {
        mRasterizerState = rasterizerState;
    }

    const Cutlass::RasterizerState& MaterialComponent::getRasterizerState() const
    {
        return mRasterizerState;
    }

    void MaterialComponent::setMultiSampleState(Cutlass::MultiSampleState multiSampleState)
    {
        mMultiSampleState = multiSampleState;
    }

    Cutlass::MultiSampleState MaterialComponent::getMultiSampleState() const
    {
        return mMultiSampleState;
    }

    void MaterialComponent::update()
    {
        //do nothing
    }

};