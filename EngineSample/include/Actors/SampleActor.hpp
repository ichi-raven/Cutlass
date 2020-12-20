#pragma once

#include "../Scenes/SceneCommonRegion.hpp"
#include "../Scenes/SceneList.hpp"

#include "../Engine/Actors/IActor.hpp"

class SampleActor : public Engine::IActor
{
    GEN_ACTOR_CLASS(SampleActor)
public:
    void test();
};