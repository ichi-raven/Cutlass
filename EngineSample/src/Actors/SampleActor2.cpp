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
	auto& loader = getSystem()->loader;

	auto mesh = addComponent<Engine::MeshComponent>();
	mSkybox = addComponent<Engine::MeshComponent>();
	if(!mesh)
		return;
	mesh->createPlane(3.f, 3.f);
	mesh->getTransform().setPos(glm::vec3(0, -0.02f, 0));
	mesh->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));

	mSkybox->createCube(300.f);
	mSkybox->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));

	auto material = addComponent<Engine::MaterialComponent>();
	loader->loadMaterialTexture("../resources/textures/sky.jpg", nullptr, material);

	auto material2 = addComponent<Engine::MaterialComponent>();
	loader->loadMaterialTexture("../resources/textures/texture.png", nullptr, material2);


	getSystem()->renderer->addStaticMesh(mesh, material2, false, true);
	getSystem()->renderer->addStaticMesh(mSkybox, material, false, false, false);
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