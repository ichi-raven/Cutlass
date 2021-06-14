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
	//loader->load(Resource::Model::genPath(Resource::Model::testGLTF).c_str(), mMesh, material);

	//mesh
	mMesh->createCube(1.f);

	auto VS = Cutlass::Shader(Resource::Shader::genPath(Resource::Shader::objVert), "main");
	auto FS = Cutlass::Shader(Resource::Shader::genPath(Resource::Shader::frag), "main");

	material->setVS(VS);
	material->setFS(FS);

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
	//他アクタに関連する処理用,関数名はUnity準拠
}

void SampleActor::update()
{
	auto&& context = getContext();
	auto camera = getComponent<Engine::CameraComponent>().value();

	{//input control
		constexpr float speed = 20;
		glm::vec3 vel(0.f);
		float rot = 0;
		auto& transform = mMesh->getTransform();

		if (context->getKey(Key::W))
			vel.z = -speed;
		if (context->getKey(Key::S))
			vel.z = speed;
		if (context->getKey(Key::A))
			vel.x = -speed;
		if (context->getKey(Key::D))
			vel.x = speed;
		if (context->getKey(Key::Up))
			vel.y = speed;
		if (context->getKey(Key::Down))
			vel.y = -speed;
		if (context->getKey(Key::Left))
			rot = -10;
		if (context->getKey(Key::Right))
			rot = 10;
		if (context->getKey(Key::Space))
		{
			transform.setRotation(glm::vec3(0, 0, 1.f), 90.f);
			transform.setRotAcc(0.f);
			camera->setLookAt(transform.getPos());
		}

		transform.setVel(vel);
		transform.setRotAxis(glm::vec3(0, 1.f, 0));
		transform.setRotAcc(rot);
	}
}