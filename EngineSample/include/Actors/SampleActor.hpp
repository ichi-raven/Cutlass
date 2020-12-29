#pragma once

#include "../Scenes/SceneCommonRegion.hpp"

#include "../Engine/Actors/IActor.hpp"

class SampleActor : public Engine::IActor<SceneCommonRegion>
{
    GEN_ACTOR_CLASS(SampleActor, SceneCommonRegion)
    
public:
    void render(const Cutlass::HRenderDST& renderDST);

private:
    Cutlass::HRenderPipeline mRenderPass;
    std::vector<Cutlass::CommandList> mCommandLists;

};