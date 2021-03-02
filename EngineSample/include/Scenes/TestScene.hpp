#pragma once

#include <Engine/Application/Application.hpp>
#include "SceneCommonRegion.hpp"
#include "SceneList.hpp"

class TestScene : public Engine::Scene<SceneList, MyCommonRegion>
{
	GEN_SCENE(TestScene, SceneList, MyCommonRegion)

private:
	void render();

	std::shared_ptr<SampleActor> mSampleActor;
};