#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <Engine/Application/ActorsInScene.hpp>

#include <cassert>
#include <memory>

SampleActor2::~SampleActor2()
{

}

void SampleActor2::awake()
{
	auto mesh = addComponent<Engine::MeshComponent>();
	if(!mesh)
		return;
	mesh->createCube(*getContext(), 1.f);
}

void SampleActor2::init()
{
	assert(getActor<SampleActor>("SampleActor"));
}

void SampleActor2::update()
{
    auto sa2 = getActors().getActor<SampleActor2>("SampleActor2").value();
    auto meshcmp = sa2->getComponent<Engine::MeshComponent>().value();
}

void SampleActor2::test2()
{
	assert(!"succeeded test2!\n");
}