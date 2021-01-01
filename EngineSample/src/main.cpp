#include <Scenes/SceneList.hpp>
#include <Scenes/SceneCommonRegion.hpp>
#include <Scenes/TestScene.hpp>

#include <Engine/Application/Application.hpp>

#include <Cutlass.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <iostream>
#include <array>
#include <chrono>
#include <numeric>

int main()
{
	//定数
	constexpr uint16_t windowWidth = 800;
	constexpr uint16_t windowHeight = 600;
	constexpr uint16_t frameCount = 3;

	//アプリケーション実体作成
	Engine::Application<SceneList, SceneCommonRegion> app
	(
		Cutlass::InitializeInfo("testApp", true), 
		{Cutlass::WindowInfo(windowWidth, windowHeight, frameCount, "testAppWindow", false, true)}
	);
	

	//コンテキスト取得
	//auto& context = app.mCommonRegion->context;
	
	// {//初期化
	// 	Cutlass::InitializeInfo ii("testApp", true);
	// 	if(Cutlass::Result::eSuccess != context.initialize(ii))
	// 		std::cerr << "Failed to initialize!\n";
	// }

	// Cutlass::HWindow window;
	// {//window作成
	// 	Cutlass::WindowInfo(windowWidth, windowHeight, frameCount, "testAppWindow", false, true);
	// 	if (Cutlass::Result::eSuccess != context.createWindow(wi, window))
	// 		std::cerr << "Failed to create window!\n";
	// }

	//注意 : 処理順序を変更するとcontextの初期化忘れが発生する可能性があります
	
	//情報セット
	app.mCommonRegion->width = windowWidth;
	app.mCommonRegion->height = windowHeight;
	app.mCommonRegion->frameCount = frameCount;
	// app.mCommonRegion->window = window;

	//シーン追加
	app.addScene<TestScene>(SceneList::eTest);
	
	//このシーンから開始, Scene::initを実行
	app.init(SceneList::eTest);

	{//メインループ
		uint32_t frame = 0;
		std::array<double, 10> times;//10F平均でFPSを計測
		std::chrono::high_resolution_clock::time_point now, prev = std::chrono::high_resolution_clock::now();

		while (!app.endAll())
		{
			{//frame数, fps表示
                now = std::chrono::high_resolution_clock::now();
                times[frame % 10] = app.mCommonRegion->deltatime = std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count() / 1000000.;
                std::cerr << "now frame : " << frame << "\n";
                std::cerr << "FPS : " << 1. / (std::accumulate(times.begin(), times.end(), 0.) / 10.) << "\n";
            }

			//アプリケーション更新
			app.update();

			{//フレーム, 時刻更新
                ++frame;
                prev = now;
            }
		}
	}

	return 0;
}