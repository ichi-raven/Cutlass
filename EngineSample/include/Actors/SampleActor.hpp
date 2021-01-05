#pragma once

#include "../Scenes/SceneCommonRegion.hpp"

#include "../Engine/Actors/IActor.hpp"

class SampleActor : public Engine::IActor<MyCommonRegion>
{
    GEN_ACTOR(SampleActor, MyCommonRegion)
    
public:

private:
    
};