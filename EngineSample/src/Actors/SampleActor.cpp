#include <Actors/SampleActor.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>

SampleActor::~SampleActor()
{

}

void SampleActor::init(INIT_ARG_ACTOR(SceneCommonRegion, actors, scr))
{
	auto mesh = addComponent<Engine::MeshComponent>();
	auto mesh2 = addComponent<Engine::MeshComponent>();
	if(!mesh || !mesh2)
		return;
	mesh->createCube(scr->context, 1.f);
}

void SampleActor::update(UPDATE_ARG_ACTOR(SceneCommonRegion, actors, scr))
{
	render()
}

void SampleActor::render(const Cutlass::HRenderDST& renderDST)
{
	
}