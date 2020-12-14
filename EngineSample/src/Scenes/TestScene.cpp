#include <Scenes/TestScene.hpp>

#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>
#include <iostream>

void TestScene::init()
{
	addActor<SampleActor>("SampleActor");
	addActor<SampleActor2>("SampleActor2");

	initActors();
}

void TestScene::update()
{
	auto& context = Cutlass::Context::getInstance();

	std::cout << "deltatime : " << getCommonRegion()->deltatime << "\n";

	//ESCで終わる
	if(context.getKey(Cutlass::Key::Escape))
		exitApplication();

	updateActors();
	render();
}

void TestScene::render()
{
	std::vector<std::shared_ptr<MeshComponent>> pRenderMeshs;
	const auto& lmdSearchRenderebleActors = [&](std::shared_ptr<IActor> actor)
	{
		if(auto cmp = actor->getComponent<MeshComponent>())
		{
			if(!cmp->getVisible())
				return;
			
			pRenderMeshs.emplace_back(cmp);
		}
	};

	getActorsInScene().forEachActor(lmdSearchRenderebleActors);

	
}
