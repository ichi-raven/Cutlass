#include <Actors/SampleActor.hpp>

#include <Engine/Components/MeshComponent.hpp>

#include <cassert>
#include <iostream>

#include <Engine/System/System.hpp>
#include <Engine/Application/ActorsInScene.hpp>

#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/CameraComponent.hpp>

#include <Engine/Utility/Transform.hpp>

SampleActor::~SampleActor()
{

}

void SampleActor::awake()
{
	auto& renderer = getSystem()->mRenderer;

	auto mesh = addComponent<Engine::MeshComponent>();
	auto material = addComponent<Engine::MaterialComponent>();
	auto camera = addComponent<Engine::CameraComponent>();
	mesh->createCube(*getContext(), 1.f);

	Engine::Transform transform;
	transform.setPos(glm::vec3(0, 0, 0));
	camera->setTransform(transform);
	camera->setViewParam(glm::vec3(0, 0, -10.f));
	camera->setProjectionParam(glm::radians(45.f), getCommonRegion()->width, getCommonRegion()->height, 1.f, 100.f);

	renderer->setCamera(camera);

	renderer->regist(mesh, material);
}

void SampleActor::init()
{
	assert(getActor<SampleActor2>("SampleActor2") != std::nullopt);
}

void SampleActor::update()
{
	
}