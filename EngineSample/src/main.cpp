#include <Scenes/SceneList.hpp>
#include <Scenes/SceneCommonRegion.hpp>
#include <Scenes/TestScene.hpp>

#include <Engine/Application/Application.hpp>

#include <Cutlass/Cutlass.hpp>

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

	//コンテキスト取得
	auto& context = Cutlass::Context::getInstance();
	
	{//初期化
		Cutlass::InitializeInfo ii("testApp", true);
		if(Cutlass::Result::eSuccess != context.initialize(ii))
			std::cerr << "failed to initialize!\n";
	}

	Cutlass::HWindow window;
	Cutlass::HRenderDST rdst;
	{//window, 描画対象オブジェクト作成
		Cutlass::WindowInfo wi(windowWidth, windowHeight, frameCount, "testAppWindow", false, true);
		if (Cutlass::Result::eSuccess != context.createWindow(wi, window))
			std::cerr << "failed to create window!\n";

		if (Cutlass::Result::eSuccess != context.createRenderDST(window, true, rdst))
			std::cerr << "failed to create frame buffer!\n";

	}

	//注意 : 処理順序を変更するとcontextの初期化忘れが発生する可能性があります
	//初期化後にアプリケーション実体を構築すれば、コンストラクタでもcontextを使用可能です
	
	//アプリケーション実体
	Application<SceneList, SceneCommonRegion> app;

	{
		//情報セット
		app.mCommonRegion->width = windowWidth;
		app.mCommonRegion->height = windowHeight;
		app.mCommonRegion->frameCount = frameCount;
		app.mCommonRegion->window = window;
		app.mCommonRegion->frameBuffer = rdst;
		
		//シーン追加
		app.addScene<TestScene>(SceneList::eTest);
		
		//このステートで開始
		app.init(SceneList::eTest);
	}

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
                std::cerr << "fps : " << 1. / (std::accumulate(times.begin(), times.end(), 0.) / 10.) << "\n";
            }

			//入力更新
			if (Cutlass::Result::eSuccess != context.updateInput())
				std::cerr << "failed to update input!";

			//アプリケーション更新
			app.update();

			{//フレーム, 時刻更新
                ++frame;
                prev = now;
            }
		}
	}

	//破棄は明示的に
	context.destroy();

	return 0;
}