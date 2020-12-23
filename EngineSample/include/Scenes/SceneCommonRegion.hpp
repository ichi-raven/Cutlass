#pragma once

#include <Cutlass/Cutlass.hpp>

struct SceneCommonRegion//下手に中身を改変しようとしないほうが良いと思われる
{
	//Noncopyableなので注意
	Cutlass::Context context;

	Cutlass::HWindow window;
	Cutlass::HRenderDST frameBuffer;

	uint16_t width;
	uint16_t height;
	uint16_t frameCount;

	double deltatime;
};