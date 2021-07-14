#include <Actors/SampleActor.hpp>
#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/MaterialComponent.hpp>

#include <Engine/Application/ActorsInScene.hpp>
#include <Engine/System/System.hpp>
#include <Engine/System/Renderer.hpp>

#include <cassert>
#include <memory>

SampleActor2::~SampleActor2()
{

}

void SampleActor2::awake()
{
	// auto mesh = addComponent<Engine::MeshComponent>();
	// if(!mesh)
	// 	return;
	// mesh->createPlane(10.f, 10.f);
	// mesh->getTransform().setPos(glm::vec3(0, -1.5f, -2.f));

	// auto material = addComponent<Engine::MaterialComponent>();

	// getSystem()->mRenderer->regist(mesh, material);
}

void SampleActor2::init()
{
	assert(getActor<SampleActor>("SampleActor"));
}

void SampleActor2::update()
{

}

void SampleActor2::test2()
{
	assert(!"succeeded test2!\n");
}