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

#include <Engine/Resources/Resources.hpp>

SampleActor::~SampleActor()
{

}

void SampleActor::awake()
{
	static auto& renderer = getSystem()->mRenderer;

	mMesh = addComponent<Engine::MeshComponent>();
	auto material = addComponent<Engine::MaterialComponent>();
	auto camera = addComponent<Engine::CameraComponent>();
	mMesh->createCube(*getContext(), 1.f);
	mMesh->getTransform().setPos(glm::vec3(0, 0, -2.f));

	camera->getTransform().setPos(glm::vec3(0, 0, 10.f));
	camera->setViewParam(glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
	camera->setProjectionParam(glm::radians(45.f), getCommonRegion()->width, getCommonRegion()->height, 0.1f, 1e6);

	renderer->setCamera(camera);

	renderer->regist(mMesh, material);
}

void SampleActor::init()
{
	//assert(getActor<SampleActor2>("SampleActor2") != std::nullopt);
}

void SampleActor::update()
{
	auto&& context = getContext();

	{
		constexpr float speed = 30;
		glm::vec3 vel(0.f);
		float rot = 0;
		Engine::Transform& transform = mMesh->getTransform();

		if (context->getKey(Cutlass::Key::W))
			vel.z = -speed;
		if (context->getKey(Cutlass::Key::S))
			vel.z = speed;
		if (context->getKey(Cutlass::Key::A))
			vel.x = -speed;
		if (context->getKey(Cutlass::Key::D))
			vel.x = speed;
		if (context->getKey(Cutlass::Key::Up))
			vel.y = speed;
		if (context->getKey(Cutlass::Key::Down))
			vel.y = -speed;
		if(context->getKey(Cutlass::Key::Left))
			rot = -10;
		if(context->getKey(Cutlass::Key::Right))
			rot = 10;
		if(context->getKey(Cutlass::Key::Space))
		{
			transform.setRotation(0.f);
			transform.setRotAcc(0.f);
		}

		transform.setVel(vel);
		transform.setRotVel(rot);
		//std::cout << "cpu pos : " << mMesh->getTransform().getPos().x << "," << mMesh->getTransform().getPos().y << "," << mMesh->getTransform().getPos().z << "\n";

	}
}