#include <Scenes/TestScene.hpp>

#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>
#include <iostream>

TestScene::~TestScene()
{
	
}

void TestScene::init()
{
	addActor<SampleActor2>("SampleActor2");
	addActor<SampleActor>("SampleActor");
}

void TestScene::update()
{
	auto context = getContext();

	std::cout << "deltatime : " << getCommonRegion()->deltatime << "\n";

	//Escキーで終わる
	if(context->getKey(Cutlass::Key::Escape))
		exitApplication();
	
	render();
}

void TestScene::render()
{

}
