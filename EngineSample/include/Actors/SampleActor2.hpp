#pragma once

#include "../Scenes/SceneCommonRegion.hpp"
#include "../Scenes/SceneList.hpp"

#include "../Engine/Actors/IActor.hpp"

class SampleActor2 : public Engine::IActor<MyCommonRegion>
{
    GEN_ACTOR(SampleActor2, MyCommonRegion)

public:
    void test2();
};