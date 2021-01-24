#include <Actors/SampleActor.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>
#include <iostream>

#include <Engine/System/System.hpp>
#include <Engine/Application/ActorsInScene.hpp>

#include <Actors/SampleActor2.hpp>

SampleActor::~SampleActor()
{

}

void SampleActor::init()
{
	auto mesh = addComponent<Engine::MeshComponent>();
	auto mesh2 = addComponent<Engine::MeshComponent>();
	if(!mesh || !mesh2)
		return;
	mesh->createCube(*getContext(), 1.f);

	assert(getActors().getActor<SampleActor2>("SampleActor2"));
}

void SampleActor::update()
{
	
}