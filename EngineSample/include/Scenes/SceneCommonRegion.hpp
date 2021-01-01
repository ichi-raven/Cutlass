#pragma once

#include <cstdint>

struct SceneCommonRegion
{
	uint16_t width;
	uint16_t height;
	uint16_t frameCount;

	double deltatime;
};