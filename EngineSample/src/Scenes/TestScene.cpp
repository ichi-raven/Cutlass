#include <Scenes/TestScene.hpp>

#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>
#include <iostream>

void TestScene::init()
{
	addActor<SampleActor>("SampleActor");
	//addActor<SampleActor2>("SampleActor2");
}

void TestScene::update()
{
	auto& context = Cutlass::Context::getInstance();

	std::cout << "deltatime : " << getCommonRegion()->deltatime << "\n";

	//ESCで終わる
	if(context.getKey(Cutlass::Key::Escape))
		exitApplication();
	render();
}

void TestScene::render()
{
	std::vector<std::shared_ptr<Engine::MeshComponent>> pRenderMeshs;
	const auto& lmdSearchRenderebleActors = [&](std::shared_ptr<Engine::IActor> actor)
	{
		if(auto&& mesh = actor->getComponent<Engine::MeshComponent>())
		if(auto&& material = actor->getComponent<Engine::MaterialComponent>())
		if(!mesh.value()->getVisible())
		{
				mRenderer.addMesh(mesh.value(), material.value());
		}

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

	mRenderer.render(getCommonRegion()->frameBuffer);
}
