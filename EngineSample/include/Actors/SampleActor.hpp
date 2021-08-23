#pragma once

#include "../Scenes/SceneCommonRegion.hpp"

#include "../Engine/Actors/IActor.hpp"

namespace Engine
{
    class MeshComponent;
    class SkeletalMeshComponent;
    class LightComponent;
}


class SampleActor : public Engine::IActor<MyCommonRegion>
{
    GEN_ACTOR(SampleActor, MyCommonRegion)
    
public:

private:
    std::shared_ptr<Engine::SkeletalMeshComponent> mMesh;
    std::shared_ptr<Engine::LightComponent> mLight;
};