#pragma once

#include <Engine/Application/Application.hpp>
#include "SceneCommonRegion.hpp"
#include "SceneList.hpp"


class TestScene : public Scene<SceneList, SceneCommonRegion>
{
	GEN_SCENE_HEADER_CLASS
private:
	void render();
};