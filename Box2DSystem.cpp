#include "pch.h"
#include "GGScene.h"
#include <Box2D/Box2D.h>
#include "Box2DSystem.h"

void Box2DComponent::add(GGScene*, entt::registry&, entt::entity, nlohmann::json&)
{

	return;
}

void Box2DSystem::update(GGScene* scene)
{
	auto B = world->GetBodyList();

	while (B)
	{
		auto temp = B;
		auto UD = B->GetUserData();
		B = B->GetNext();
		if (!UD) continue;

		entt::entity E = (entt::entity)(u64)UD;
		if (!scene->entities.has<Transform2D>(E)) continue;

		auto pos = temp->GetPosition();
		auto ang = temp->GetAngle();
		scene->entities.replace<Transform2D>(E, pos.x, pos.y, ang);
	}

	return;
}

void Box2DSystem::register_components(GGScene* scene)
{
	//scene->components.insert(std::make_pair("physic2d", new Box2DComponent()));
	return;
}

Box2DSystem::Box2DSystem() : world(new b2World({0, 9.0f}))
{
	return;
}

SystemInterface* Box2DSystem_factory() { return new Box2DSystem; }

REGISTER_SYSTEM(Box2D)