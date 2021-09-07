#include <Actors/SampleActor.hpp>

#include <Engine/Components/MeshComponent.hpp>
#include <Engine/Components/SkeletalMeshComponent.hpp>

#include <cassert>
#include <iostream>

#include <Engine/System/System.hpp>
#include <Engine/Application/ActorsInScene.hpp>

#include <Actors/SampleActor2.hpp>

#include <Engine/Components/MaterialComponent.hpp>
#include <Engine/Components/CameraComponent.hpp>
#include <Engine/Components/LightComponent.hpp>

#include <Engine/Utility/Transform.hpp>

#include <Engine/Resources/Resources.hpp>

using namespace Cutlass;
using namespace Engine;

SampleActor::~SampleActor()
{

}

void SampleActor::awake()
{
	auto&& renderer = getSystem()->renderer;
	auto&& loader = getSystem()->loader;

	//コンポーネント追加
	mMesh = addComponent<Engine::SkeletalMeshComponent>();
	auto material = addComponent<Engine::MaterialComponent>();
	auto camera = addComponent<Engine::CameraComponent>();
	mLight = addComponent<Engine::LightComponent>();
	auto light2 = addComponent<Engine::LightComponent>();
	//ロード
	loader->loadSkeletal("../resources/models/CesiumMan/glTF/CesiumMan.gltf", mMesh, material);
	//loader->loadSkeletal("../resources/models/ß®Ýñ - Rs[/xisamoTEST2.fbx", mMesh, material);

	mMesh->setAnimationIndex(0);
	//mMesh->setTimeScale(10);
	//mMesh->getTransform().setScale(glm::vec3(0.005, 0.005, 0.005));
	//mMesh->changeDefaultAxis(glm::vec3(0, 0, 1.f), glm::vec3(0, 1.f, 0), glm::vec3(1.f, 0, 0));
	mMesh->changeDefaultAxis(glm::vec3(1.f, 0, 0), glm::vec3(0, 0, -1.f), glm::vec3(0, 1.f, 0));
	std::cerr << mMesh->getVertexNum() << "\n";

	//camera
	camera->getTransform().setPos(glm::vec3(0, 2.f, 6.f));
	camera->setViewParam(glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
	camera->setProjectionParam(45.f, getCommonRegion()->width, getCommonRegion()->height, 0.1f, 1000.f);

	//light
	auto lightColor = glm::vec4(0.3f, 0.3f, 0.3f, 0);
	auto lightDir = glm::vec3(-1.f, -1.f, -1.f);
	mLight->setAsDirectionalLight(lightColor, lightDir);
	lightColor = glm::vec4(0.5f, 0.5f, 0.8f, 0);
	light2->setAsDirectionalLight(lightColor, lightDir);

	//renderer
	renderer->setCamera(camera);
	//renderer->addLight(mLight);
	renderer->addLight(light2);
	renderer->addSkeletalMesh(mMesh, material, true, false);
}

void SampleActor::init()
{
	//他アクタに関連する処理用,関数名はUnity準拠
	assert(getComponents<Engine::LightComponent>().value().size() == 2);
	assert(getActor<SampleActor2>("SampleActor2"));
}

void SampleActor::update()
{
	auto&& input = getSystem()->input;
	auto camera = getComponent<Engine::CameraComponent>().value();

	{//input control
		constexpr float speed = 2;
		glm::vec3 vel(0.f);
		float rot = 0;
		auto& transform = mMesh->getTransform();

		if (input->getKey(Key::W))
			vel.z = -speed;
		if (input->getKey(Key::S))
			vel.z = speed;
		if (input->getKey(Key::A))
			vel.x = -speed;
		if (input->getKey(Key::D))
			vel.x = speed;
		if (input->getKey(Key::Up))
			vel.y = speed;
		if (input->getKey(Key::Down))
			vel.y = -speed;
		if (input->getKey(Key::Left))
			rot = -10;
		if (input->getKey(Key::Right))
			rot = 10;
		if (input->getKey(Key::Space))
		{
			transform.setRotation(glm::vec3(0, 0, 1.f), 90.f);
			transform.setRotVel(0.f);
			camera->setLookAt(transform.getPos());
		}

		transform.setVel(vel);
		transform.setRotAxis(glm::vec3(0, 1.f, 0));
		transform.setRotAcc(rot);
		glm::vec3 pos = transform.getPos();
		//transform.setPos(glm::vec3(pos.x, pos.y + 0.02 * sin(getCommonRegion()->frame * 3.14159 / 60), pos.z));
	}

	if(getCommonRegion()->frame % 120 == 0)
	{
		srand(time(NULL));
		float random[] = {1.f * rand() / RAND_MAX, 1.f * rand() / RAND_MAX, 1.f * rand() / RAND_MAX};
		float random2[] = {1.f * (rand() % 10 + 1) - 5, 1.f * (rand() % 5 + 5), 1.f * (rand() % 10 + 1) - 5};
		auto lightDir = glm::vec3(random2[0], random2[1], random2[2]);
		auto lightColor = glm::vec4(random[0], random[1], random[2], 1.f);
		mLight->setAsDirectionalLight(mLight->getColor(), lightDir);
	}
}