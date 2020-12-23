#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <Engine/Application/ActorsInScene.hpp>

#include <cassert>
#include <memory>

SampleActor2::~SampleActor2()
{

}

void SampleActor2::init(INIT_ARG_ACTORS(SceneCommonRegion, actors, scr))
{
	auto mesh = addComponent<Engine::MeshComponent>(std::ref(scr->context));
	if(!mesh)
		return;
	mesh->createCube(1.f);
}

void SampleActor2::update(UPDATE_ARG_ACTORS(SceneCommonRegion, actors, scr))
{
    auto sa2 = actors.getActor<SampleActor2>("SampleActor2").value();
    auto meshcmp = sa2->getComponent<Engine::MeshComponent>().value();
}

void SampleActor2::test2()
{
	assert(!"succeeded test2!\n");
}