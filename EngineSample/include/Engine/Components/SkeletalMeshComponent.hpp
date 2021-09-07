#pragma once

#include <Cutlass.hpp>
#include <glm/glm.hpp>

#include <typeinfo>
#include <optional>

#include "MeshComponent.hpp"

#include "../Utility/Transform.hpp"

#include <assimp/scene.h>

namespace Engine
{
    class SkeletalMeshComponent : public MeshComponent
    {
    public:

        struct Bone
        {
            glm::mat4 offset;
            glm::mat4 transform;
        };

        struct Skeleton
        {
            Skeleton()
            : defaultAxis(glm::mat4(1.f))
            {

            }

            void setAIScene(const std::shared_ptr<const aiScene>& s)
            {
                scene = s;
            }

            void setGlobalInverse(const glm::mat4& inv)
            {
                mGlobalInverse = inv;
            }

            void update(float second, size_t animationIndex = 0);

            std::vector<Bone> bones;
            std::map<std::string, size_t> boneMap;
            glm::mat4 defaultAxis;
        private:
            void traverseNode(float timeInAnim, size_t animationIndex, const aiNode* node, glm::mat4 parentTransform);
            std::shared_ptr<const aiScene> scene;
            glm::mat4 mGlobalInverse;
        };

        SkeletalMeshComponent();
        virtual ~SkeletalMeshComponent();

        void create
        (
            const std::vector<Vertex>& vertices,
            const std::vector<uint32_t>& indices,
            const Skeleton& skeleton
        );

        const std::vector<Bone>& getBones() const;

        //特定の軸を差し替える 例 : Z_UP->Y_UPなら({1,0,0}, {0,0,1}, {0,1,0})
        void changeDefaultAxis(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z);

        void setAnimationIndex(uint32_t animation);
        const std::optional<uint32_t>& getAnimationIndex() const;

        void setTimeScale(float timescale);
        float getTimeScale() const;

        virtual void update() override;

    private:
        std::chrono::high_resolution_clock::time_point mStart;
        std::optional<Skeleton> mSkeleton;
		std::optional<uint32_t> mAnimationIndex;
        float mTimeScale;
    };
};