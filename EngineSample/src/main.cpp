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
	Engine::Application<SceneList, MyCommonRegion> app
	(
		Cutlass::InitializeInfo("testApp", true), 
		{Cutlass::WindowInfo(windowWidth, windowHeight, frameCount, "testAppWindow", false, true)}
	);

	//注意 : 処理順序を変更するとcontextの初期化忘れが発生する可能性があります
	
	//情報セット
	app.mCommonRegion->width = windowWidth;
	app.mCommonRegion->height = windowHeight;
	app.mCommonRegion->frameCount = frameCount;

	//シーン追加
	app.addScene<TestScene>(SceneList::eTest);
	
	//このシーンから開始, Scene::initを実行
	app.init(SceneList::eTest);

	{//メインループ
		uint32_t frame = 0;
		std::array<float, 10> times;//10f平均でfpsを計測
		std::chrono::high_resolution_clock::time_point now, prev = std::chrono::high_resolution_clock::now();

		while (!app.endAll())
		{
			{//frame数, fps
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