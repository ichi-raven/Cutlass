#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/TransformComponent.hpp>

#include <Engine/Application/ActorsInScene.hpp>

#include <cassert>
#include <memory>

SampleActor2::~SampleActor2()
{

}

void SampleActor2::init(INIT_ARG_ACTORS(actors))
{
	addComponent<MeshComponent>();
	addComponent<TransformComponent>();
	auto mesh = getComponent<MeshComponent>();
	if(!mesh)
		return;
	mesh.value()->loadCube(1.f);
}

void SampleActor2::update(UPDATE_ARG_ACTORS(actors))
{
    auto sa2 = actors.getActor<SampleActor2>("SampleActor2").value();
    auto meshcmp = sa2->getComponent<MeshComponent>().value();

	updateComponents();
}

void SampleActor2::test2()
{
	assert(!"succeeded test2!\n");
}