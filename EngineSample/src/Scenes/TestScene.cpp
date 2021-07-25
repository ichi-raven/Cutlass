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
	mSampleActor = addActor<SampleActor>("SampleActor");
	auto&& sa2 = addActor<SampleActor2>("SampleActor2");

	mSampleActor->init();
	sa2->init();
}

void TestScene::update()
{
	auto&& context = getContext();
	auto&& renderer = getSystem()->mRenderer;

	std::cout << "deltatime : " << getCommonRegion()->deltatime << "\n";

	//Escキーで終わる
	if(context->getKey(Cutlass::Key::Escape))
		exitApplication();

	renderer->build();
	renderer->render();

	//assert(!"fjeiaje");
}

