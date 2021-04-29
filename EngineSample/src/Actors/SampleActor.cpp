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

using namespace Cutlass;
using namespace Engine;

SampleActor::~SampleActor()
{

}

void SampleActor::awake()
{
	auto&& renderer = getSystem()->mRenderer;
	auto&& loader = getSystem()->mLoader;

	//コンポーネント追加
	mMesh = addComponent<Engine::MeshComponent>();
	auto material = addComponent<Engine::MaterialComponent>();
	auto camera = addComponent<Engine::CameraComponent>();

	//ロード
	loader->load(Engine::Resource::testGLTF.data());
	loader->getMesh(mMesh);

	//mesh
	mMesh->getTransform().setPos(glm::vec3(0, 0, -2.f));

	material->setVS(Cutlass::Shader(static_cast<std::string>(Resource::shaderDir) + std::string("MeshWithMaterial/gltfvert.spv"), "main"));

	//camera
	camera->getTransform().setPos(glm::vec3(0, 0, 10.f));
	camera->setViewParam(glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
	camera->setProjectionParam(45.f, getCommonRegion()->width, getCommonRegion()->height, 0.1f, 1e6);

	//renderer
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
	auto camera = getComponent<Engine::CameraComponent>().value();

	{
		constexpr float speed = 20;
		glm::vec3 vel(0.f);
		float rot = 0;
		Engine::Transform& transform = mMesh->getTransform();
		transform.setRotAxis(glm::vec3(1.f, 0, 0));

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
			transform.setRotation(glm::vec3(0, 0, 1.f), 90.f);
			transform.setRotAcc(0.f);
			camera->setLookAt(transform.getPos());
		}

		transform.setVel(vel);
		transform.setRotAxis(glm::vec3(0, 1.f, 0));
		transform.setRotVel(rot);
		//camera->setLookAt(transform.getPos());
	}
}