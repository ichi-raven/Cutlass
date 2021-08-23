#include <Engine/Components/SkeletalMeshComponent.hpp>


#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

namespace Engine
{
    inline glm::mat4 convert4x4(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        //transpose
        for(uint8_t i = 0; i < 4; ++i)
            for(uint8_t j = 0; j < 4; ++j)
                to[i][j] = from[j][i];
        

        return to;
    } 

    inline glm::quat convertQuat(const aiQuaternion& from)
    {
        glm::quat to;
        to.w = from.w;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;

        return to;
    }

    inline glm::vec3 convertVec3(const aiVector3D& from)
    {
        glm::vec3 to;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;

        return to;
    }

    void SkeletalMeshComponent::Skeleton::update(float second, size_t animationIndex)
    {
        if(animationIndex < scene->mNumAnimations && animationIndex >= 0)
        {
            assert(!"invalid animation index!");
            return;
        }

        float updateTime = scene->mAnimations[animationIndex]->mTicksPerSecond;
        if(updateTime == 0)
        {
            assert("zero update time!");
            updateTime = 25.f;//?
        }

        traverseNode(fmod(second * updateTime, scene->mAnimations[animationIndex]->mDuration), animationIndex, scene->mRootNode, glm::mat4(1.f));
    }

    inline size_t findScale(float time, aiNodeAnim* pAnimationNode)
    {
        if(pAnimationNode->mNumScalingKeys == 1)
            return 0;
        

        for(size_t i = 0; i < pAnimationNode->mNumScalingKeys - 1; ++i)
            if(time < static_cast<float>(pAnimationNode->mScalingKeys[i + 1].mTime))
                return i;

        assert(!"failed to find scale index!");
        return 0;
    }

    inline size_t findRotation(float time, aiNodeAnim* pAnimationNode)
    {
        if(pAnimationNode->mNumRotationKeys == 1)
            return 0;

        for(size_t i = 0; i < pAnimationNode->mNumRotationKeys - 1; ++i)
            if(time < static_cast<float>(pAnimationNode->mRotationKeys[i + 1].mTime))
                return i;

        assert(!"failed to find rotation index!");
        return 0;
    }

    inline size_t findPosition(float time, aiNodeAnim* pAnimationNode)
    {
        if(pAnimationNode->mNumPositionKeys == 1)
            return 0;

        for(size_t i = 0; i < pAnimationNode->mNumPositionKeys - 1; ++i)
            if(time < static_cast<float>(pAnimationNode->mPositionKeys[i + 1].mTime))
                return i;
        
        assert(!"failed to find position index!");
        return 0;
    }

    void SkeletalMeshComponent::Skeleton::traverseNode(float timeInAnim, size_t animationIndex, const aiNode* node, glm::mat4 parentTransform)
    {
        //std::cerr << "\n\n\nprev parentTransform\n" << glm::to_string(parentTransform) << "\n";

        std::string nodeName = std::string(node->mName.C_Str());

        const aiAnimation* pAnimation = scene->mAnimations[animationIndex];

        glm::mat4 transform = convert4x4(node->mTransformation);

        // if(nodeName == "Z_UP")
        // {
        //     transform = glm::mat4(1.f);
        // }

        //std::cerr << "original transform\n" << glm::to_string(transform) << "\n";

        aiNodeAnim* pAnimationNode = nullptr;

        //find animation node
        for (size_t i = 0; i < pAnimation->mNumChannels; ++i) 
            if (std::string(pAnimation->mChannels[i]->mNodeName.data) == nodeName) 
            {
                pAnimationNode = pAnimation->mChannels[i];
                break;
            }
        

        if(pAnimationNode)
        {
            //check
            //std::cerr << pAnimationNode->mNumScalingKeys << "\n";
            //std::cerr << pAnimationNode->mNumRotationKeys << "\n";
            //std::cerr << pAnimationNode->mNumPositionKeys << "\n";
            assert(pAnimationNode->mNumScalingKeys  >= 1);
            assert(pAnimationNode->mNumRotationKeys >= 1);
            assert(pAnimationNode->mNumPositionKeys >= 1);

            //find right time scaling key
            size_t scaleIndex 	 = findScale(timeInAnim, pAnimationNode);
            size_t rotationIndex = findRotation(timeInAnim, pAnimationNode);
            size_t positionIndex = findPosition(timeInAnim, pAnimationNode);

            //std::cerr << "keys : \n";
            // std::cerr << scaleIndex << "\n";
            // std::cerr << rotationIndex << "\n";
            // std::cerr << positionIndex << "\n\n";

            glm::mat4 scale, rotation, translate;

            //scale
            if(pAnimationNode->mNumScalingKeys == 1)
                scale = glm::scale(glm::mat4(1.f), convertVec3(pAnimationNode->mScalingKeys[scaleIndex].mValue));
            else
            {
                glm::vec3&& start = convertVec3(pAnimationNode->mScalingKeys[scaleIndex].mValue);
                glm::vec3&& end   = convertVec3(pAnimationNode->mScalingKeys[scaleIndex + 1].mValue);
                float dt = pAnimationNode->mScalingKeys[scaleIndex + 1].mTime - pAnimationNode->mScalingKeys[scaleIndex].mTime;

                glm::vec3&& lerped = glm::mix(start, end, (timeInAnim - static_cast<float>(pAnimationNode->mScalingKeys[scaleIndex].mTime)) / dt);
                scale = glm::scale(glm::mat4(1.f), lerped);
                //std::cerr << "scale\n" << glm::to_string(scale) << "\n";
            }

            //rotation
            {
                glm::quat&& start = convertQuat(pAnimationNode->mRotationKeys[rotationIndex].mValue);
                glm::quat&& end   = convertQuat(pAnimationNode->mRotationKeys[rotationIndex + 1].mValue);
                float dt = pAnimationNode->mRotationKeys[rotationIndex + 1].mTime - pAnimationNode->mRotationKeys[rotationIndex].mTime;
                
                glm::quat&& slerped = glm::mix(start, end, (timeInAnim - static_cast<float>(pAnimationNode->mRotationKeys[rotationIndex].mTime)) / dt);
                rotation = glm::toMat4(glm::normalize(slerped));
                //std::cerr << "rotation\n" << glm::to_string(rotation) << "\n";
            }

            //position(translate)
            {
                glm::vec3&& start = convertVec3(pAnimationNode->mPositionKeys[positionIndex].mValue);
                glm::vec3&& end   = convertVec3(pAnimationNode->mPositionKeys[positionIndex + 1].mValue);
                float dt = pAnimationNode->mPositionKeys[positionIndex + 1].mTime - pAnimationNode->mPositionKeys[positionIndex].mTime;

                glm::vec3&& lerped =  glm::mix(start, end, (timeInAnim - static_cast<float>(pAnimationNode->mPositionKeys[positionIndex].mTime)) / dt);
                translate = glm::translate(glm::mat4(1.f), lerped);
                //std::cerr << "translate\n" << glm::to_string(translate) << "\n";
            }

            transform = translate * rotation * scale;
        }

        glm::mat4 globalTransform = parentTransform * transform;

        if(boneMap.count(nodeName) > 0)
        {
            size_t boneIndex = boneMap[nodeName];
            // std::cerr << "boneIndex " << boneIndex << "\n";
            // std::cerr << "transform\n" << glm::to_string(transform) << "\n";
            // std::cerr << "parentTransform\n" << glm::to_string(parentTransform) << "\n";
            // std::cerr << "globalTransform\n" << glm::to_string(globalTransform) << "\n";
            // std::cerr << "boneOffset\n" << glm::to_string(bones[boneIndex].offset) << "\n";
            bones[boneIndex].transform = defaultAxis * mGlobalInverse * globalTransform * bones[boneIndex].offset;
            //std::cerr << "boneTransform\n" << glm::to_string(bones[boneIndex].transform) << "\n";
        }

        for(size_t i = 0; i < node->mNumChildren; ++i)
            traverseNode(timeInAnim, animationIndex, node->mChildren[i], globalTransform);
    }

    SkeletalMeshComponent::SkeletalMeshComponent()
    {

    }

    SkeletalMeshComponent::~SkeletalMeshComponent()
    {

    }

    void SkeletalMeshComponent::create
    (
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const Skeleton& skeleton
    )
    {
        //MeshComponentと同じ
        MeshComponent::create(vertices, indices);
        mSkeleton = skeleton;
    }

    const std::vector<SkeletalMeshComponent::Bone>& SkeletalMeshComponent::getBones() const
    {
        assert(mSkeleton);
        return mSkeleton->bones;
    }

    void SkeletalMeshComponent::changeDefaultAxis(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z)
    {
        assert(mSkeleton);
        mSkeleton->defaultAxis = glm::mat4(1);
        glm::vec3 axis[] = {x, y, z};
        for(size_t i = 0; i < 3; ++i)
            for(size_t j = 0; j < 3; ++j)
                mSkeleton->defaultAxis[i][j] = axis[i][j];
    }


    void SkeletalMeshComponent::setAnimationIndex(uint32_t animationIndex)
    {
        assert(mSkeleton);
        mAnimationIndex = animationIndex;
        mStart = std::chrono::high_resolution_clock::now();
    }

    const std::optional<uint32_t>& SkeletalMeshComponent::getAnimationIndex() const
    {
        assert(mSkeleton);
        return mAnimationIndex;
    }

    void SkeletalMeshComponent::update()
    {
        
        MeshComponent::update();
        //アニメーション更新
        auto&& now = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration_cast<std::chrono::microseconds>(now - mStart).count() / 1000000.;

        if(mAnimationIndex)
        {
            std::cerr << "animation time : " << time << "\n";
            mSkeleton->update(time, mAnimationIndex.value());
        }
    }
    
}