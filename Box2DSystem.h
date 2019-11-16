#pragma once
#include "GGScene.h"

class b2World;

class Box2DSystem : public SystemInterface
{
public:
	Box2DSystem();
	void register_components(GGScene*) override;
	void update(GGScene*) override;
	std::unique_ptr<b2World> world;
};

class Box2DComponent : public ComponentInterface
{
public:
	void add(GGScene*, entt::registry&, entt::entity, nlohmann::json&) override;
};


