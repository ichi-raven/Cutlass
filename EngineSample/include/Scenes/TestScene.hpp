#pragma once

#include <Engine/Application/Application.hpp>
#include "SceneCommonRegion.hpp"
#include "SceneList.hpp"

#include <Engine/System/BaseRenderer.hpp>

class TestScene : public Engine::Scene<SceneList, SceneCommonRegion>
{
	GEN_SCENE_HEADER_CLASS
private:
	void render();
	
	Engine::BaseRenderer mRenderer;
};