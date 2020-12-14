#include "../include/Scenes/TestScene.hpp"

#include "../include/Actors/SampleActor.hpp"
#include "../include/Actors/SampleActor2.hpp"

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
	//描画する
}
