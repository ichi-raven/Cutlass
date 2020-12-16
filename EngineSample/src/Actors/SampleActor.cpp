#include <Actors/SampleActor.hpp>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/TransformComponent.hpp>

#include <cassert>

SampleActor::~SampleActor()
{

}

void SampleActor::init(INIT_ARG_ACTORS(actors))
{
	addComponent<MeshComponent>("testPath");
	addComponent<TransformComponent>();
	auto mesh = getComponent<MeshComponent>();
	if(!mesh)
		return;
	mesh.value()->loadCube(1.f);
}

void SampleActor::update(UPDATE_ARG_ACTORS(actors))
{
	updateComponents();
}

void SampleActor::test()
{
	assert(!"Succeeded test!\n");
}