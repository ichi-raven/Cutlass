#pragma once

#include <Cutlass/Cutlass.hpp>

struct SceneCommonRegion//下手に中身を改変しようとしないほうが良い
{
	//描画用データ
	Cutlass::HWindow window;
	Cutlass::HRenderDST frameBuffer;

	uint16_t width;
	uint16_t height;
	uint16_t frameCount;

	//ゲーム等データ
	double deltatime;

};