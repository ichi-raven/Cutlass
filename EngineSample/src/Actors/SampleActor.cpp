#include <Actors/SampleActor.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>

SampleActor::~SampleActor()
{

}

void SampleActor::init(INIT_ARG_ACTORS(SceneCommonRegion, actors, scr))
{
	auto mesh = addComponent<Engine::MeshComponent>(std::ref(scr->context));
	if(!mesh)
		return;
	mesh->createCube(1.f);
}

void SampleActor::update(UPDATE_ARG_ACTORS(SceneCommonRegion, actors, scr))
{
	
}

void SampleActor::test()
{
	assert(!"Succeeded test!\n");
}