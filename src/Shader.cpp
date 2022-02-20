#include "../include/Shader.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "../include/ThirdParty/spirv_reflect.h"

//spirv-reflectへの依存はShaderクラスのみとしたい

namespace Cutlass
{
    void Shader::load(std::string_view path)
    {
        load(path, "");
    }

    void Shader::load(std::string_view path, std::string_view entryPoint)
    {
        //load binary data
        mPath = std::string(path);

        std::ifstream infile(path.data(), std::ios::binary);
        assert(infile);

        mFileData.resize(uint32_t(infile.seekg(0, std::ifstream::end).tellg()));
        infile.seekg(0, std::ifstream::beg).read(mFileData.data(), mFileData.size());

        //load shader module
        SpvReflectShaderModule module = {};
        SpvReflectResult result       = spvReflectCreateShaderModule(sizeof(mFileData[0]) * mFileData.size(), mFileData.data(), &module);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::cout << "loaded shader : " << path << "\n";
        std::cout << "stage : ";

        if (strcmp(entryPoint.data(), "") == 0)
            mEntryPoint = std::string(module.entry_point_name);
        else
            mEntryPoint = entryPoint;

        switch (module.shader_stage)
        {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
                std::cout << "VS";
                break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                std::cout << "HS";
                break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                std::cout << "DS";
                break;
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
                std::cout << "GS";
                break;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
                std::cout << "PS";
                break;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
                std::cout << "CS";
                break;
            default:
                break;
        }
        std::cout << "\n";

        //get desecriptor set layout
        {
            std::vector<SpvReflectDescriptorSet*> sets;
            {
                uint32_t count = 0;
                result         = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
                assert(result == SPV_REFLECT_RESULT_SUCCESS);

                sets.resize(count);

                result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
                assert(result == SPV_REFLECT_RESULT_SUCCESS);
            }

            for (size_t i = 0; i < sets.size(); ++i)
            {
                for (size_t j = 0; j < sets[i]->binding_count; ++j)
                {
                    ShaderResourceType srt;
                    switch (sets[i]->bindings[j]->descriptor_type)
                    {
                        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                            srt = ShaderResourceType::eSampler;
                            break;
                        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                            srt = ShaderResourceType::eCombinedTexture;
                            break;
                        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                            srt = ShaderResourceType::eCombinedTexture;
                            break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE              : ; break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER       : ; break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER       : ; break;
                        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                            srt = ShaderResourceType::eUniformBuffer;
                            break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER             : ; break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC     : ; break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC     : ; break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT           : ; break;
                        //case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR : ; break;
                        default:
                            std::cout << "ERROR!\nparam : " << sets[i]->bindings[j]->descriptor_type << "\n";
                            break;
                    }
                    mResourceLayoutTable.emplace(std::pair<uint8_t, uint8_t>(sets[i]->set, sets[i]->bindings[j]->binding), srt);
                }
            }
        }

        {  //入出力変数を取得
            static auto lmdComp = [](const SpvReflectInterfaceVariable* l, const SpvReflectInterfaceVariable* r) -> bool
            {
                return l->location < r->location;
            };

            uint32_t count = 0;
            result         = spvReflectEnumerateInputVariables(&module, &count, NULL);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            std::vector<SpvReflectInterfaceVariable*> inputVars(count);
            mInputVariables.resize(count);
            result = spvReflectEnumerateInputVariables(&module, &count, inputVars.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
            if (!inputVars.empty())
                std::sort(inputVars.begin(), inputVars.end(), lmdComp);

            count  = 0;
            result = spvReflectEnumerateOutputVariables(&module, &count, NULL);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            std::vector<SpvReflectInterfaceVariable*> outputVars(count);
            mOutputVariables.resize(count);
            result = spvReflectEnumerateOutputVariables(&module, &count, outputVars.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
            if (!outputVars.empty())
                std::sort(outputVars.begin(), outputVars.end(), lmdComp);

            //input
            for (size_t i = 0; i < mInputVariables.size(); ++i)
            {
                auto inputVar = spvReflectGetInputVariableByLocation(&module, inputVars[i]->location, &result);
                if (inputVars[i]->location == UINT32_MAX)
                {
                    assert(result == SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND);
                    assert(inputVar == nullptr);
                    mInputVariables.pop_back();
                    continue;
                }
                else
                {
                    assert(result == SPV_REFLECT_RESULT_SUCCESS);
                    assert(inputVars[i] == inputVar);
                }
                (void)inputVar;

                if (inputVar->semantic != nullptr)  //nullptrならsemanticはnulloptのまま
                    mInputVariables[i].second = std::string(inputVar->semantic);

                switch (inputVar->type_description->op)
                {
                    case SpvOpTypeVector:
                        {
                            switch (inputVar->type_description->traits.numeric.scalar.width)
                            {
                                case 32:
                                    {
                                        switch (inputVar->type_description->traits.numeric.vector.component_count)
                                        {
                                            case 2:
                                                mInputVariables[i].first = ResourceType::eF32Vec2;
                                                break;
                                            case 3:
                                                mInputVariables[i].first = ResourceType::eF32Vec3;
                                                break;
                                            case 4:
                                                mInputVariables[i].first = ResourceType::eF32Vec4;
                                                break;
                                        }
                                    }
                                    break;

                                case 64:
                                    {
                                        switch (inputVar->type_description->traits.numeric.vector.component_count)
                                        {
                                            //非対応です
                                            case 2:
                                                assert(!"double2");
                                            case 3:
                                                assert(!"double3");
                                            case 4:
                                                assert(!"double4");
                                        }
                                    }
                                    break;
                            }
                        }
                        break;

                    default:
                        break;
                }
            }

            //output
            for (size_t i = 0; i < mOutputVariables.size(); ++i)
            {
                auto outputVar = spvReflectGetOutputVariableByLocation(&module, outputVars[i]->location, &result);
                if (outputVars[i]->location == UINT32_MAX)
                {
                    assert(result == SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND);
                    assert(outputVar == nullptr);
                    mOutputVariables.pop_back();
                    continue;
                }
                else
                {
                    assert(result == SPV_REFLECT_RESULT_SUCCESS);
                    assert(outputVars[i] == outputVar);
                }
                (void)outputVar;

                if (outputVars[i]->semantic != nullptr)  //nullptrならsemanticはnulloptのまま
                    mOutputVariables[i].second = std::string(outputVars[i]->semantic);

                switch (outputVar->type_description->op)
                {
                    case SpvOpTypeVector:
                        {
                            switch (outputVar->type_description->traits.numeric.scalar.width)
                            {
                                case 32:
                                    {
                                        switch (outputVar->type_description->traits.numeric.vector.component_count)
                                        {
                                            case 2:
                                                mOutputVariables[i].first = ResourceType::eF32Vec2;
                                                break;
                                            case 3:
                                                mOutputVariables[i].first = ResourceType::eF32Vec3;
                                                break;
                                            case 4:
                                                mOutputVariables[i].first = ResourceType::eF32Vec4;
                                                break;
                                        }
                                    }
                                    break;

                                case 64:
                                    {
                                        switch (outputVar->type_description->traits.numeric.vector.component_count)
                                        {
                                            //非対応です
                                            case 2:
                                                assert(!"double2");
                                            case 3:
                                                assert(!"double3");
                                            case 4:
                                                assert(!"double4");
                                        }
                                    }
                                    break;
                            }
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        spvReflectDestroyShaderModule(&module);
    }

    const std::vector<char>& Shader::getShaderByteCode() const
    {
        return mFileData;
    }

    std::string_view Shader::getEntryPoint() const
    {
        return mEntryPoint;
    }

    std::string_view Shader::getPath() const
    {
        return mPath;
    }

    const std::map<std::pair<uint8_t, uint8_t>, Shader::ShaderResourceType>& Shader::getLayoutTable() const
    {
        return mResourceLayoutTable;
    }

    const std::vector<std::pair<ResourceType, std::optional<std::string>>>& Shader::getInputVariables() const
    {
        return mInputVariables;
    }

    const std::vector<std::pair<ResourceType, std::optional<std::string>>>& Shader::getOutputVariables() const
    {
        return mOutputVariables;
    }

    void ShaderResourceSet::bind(uint8_t binding, const HBuffer& handle)
    {
        uniformBuffers.emplace(binding, handle);
    }

    void ShaderResourceSet::bind(uint8_t binding, const HTexture& handle)
    {
        combinedTextures.emplace(binding, handle);
    }

    const std::map<uint8_t, HBuffer>& ShaderResourceSet::getUniformBuffers() const
    {
        return uniformBuffers;
    }

    const std::map<uint8_t, HTexture>& ShaderResourceSet::getCombinedTextures() const
    {
        return combinedTextures;
    }
};  // namespace Cutlass