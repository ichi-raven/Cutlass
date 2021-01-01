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
}

void TestScene::update()
{
	auto context = getContext();

	std::cout << "deltatime : " << getCommonRegion()->deltatime << "\n";

	//終わる
	if(context->getKey(Cutlass::Key::Escape))
		exitApplication();
	
	render();
}

void TestScene::render()
{

}
