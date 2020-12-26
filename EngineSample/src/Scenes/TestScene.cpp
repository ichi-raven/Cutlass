#include <Scenes/TestScene.hpp>

#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>
#include <iostream>

TestScene::TestScene()
{

}

void TestScene::init()
{
	mRenderer.init(getCommonRegion()->context);
	addActor<SampleActor>("SampleActor");
	addActor<SampleActor2>("SampleActor2");
}

void TestScene::update()
{
	auto& context = getCommonRegion()->context;

	std::cout << "deltatime : " << getCommonRegion()->deltatime << "\n";

	//終わる
	if(context.shouldClose() || context.getKey(Cutlass::Key::Escape))
		exitApplication();
	
	render();
}

void TestScene::render()
{
	//内部では連続的なメモリ上でキャッシュされているため、線形の処理もある程度高速です
	const auto&& lmdSearchRenderebleActors = [&](std::shared_ptr<Engine::IActor<SceneCommonRegion>> actor)
	{
		// if(auto&& mesh = actor->getComponent<Engine::MeshComponent>())
		// if(auto&& material = actor->getComponent<Engine::MaterialComponent>())
		// if(!mesh.value()->getVisible())
		// {
		// 	mRenderer.addMesh(mesh.value(), material.value());
		// }

		// if(auto&& camera = actor->getComponent<Engine::CameraComponent>())
		// {
		// 	//CameraComponentの有効フラグをチェック
		// 	if(false)
		// 		mRenderer.addCamera(camera.value());
		// }

		// if(auto&& light = actor->getComponent<Engine::LightComponent>())
		// {
		// 	//LightComponentの有効フラグをチェック
		// 	if(false)
		// 		mRenderer.addLight(light.value());
		// }
	};

	getActorsInScene().forEachActor(lmdSearchRenderebleActors);

	mRenderer.render(getCommonRegion()->context);
}
