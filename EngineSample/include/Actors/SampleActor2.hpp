#pragma once

#include "../Scenes/SceneCommonRegion.hpp"
#include "../Scenes/SceneList.hpp"

#include "../Engine/Actors/IActor.hpp"

class SampleActor2 : public IActor
{
    GEN_ACTOR_CLASS(SampleActor2)
public:
    void test2();
};