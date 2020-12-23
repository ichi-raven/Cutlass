#pragma once

#include "../Scenes/SceneCommonRegion.hpp"
#include "../Scenes/SceneList.hpp"

#include "../Engine/Actors/IActor.hpp"

class SampleActor2 : public Engine::IActor<SceneCommonRegion>
{
    GEN_ACTOR_CLASS(SampleActor2, SceneCommonRegion)
public:
    void test2();
};