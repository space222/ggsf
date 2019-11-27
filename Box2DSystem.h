#pragma once
#include "GGScene.h"

struct B2DPhysics
{
	int type;
	bool sensor;
	std::vector<b2Shape*> shapes;
	std::vector<b2Vec2> shape_pos;
	b2Body* body;
	float friction, restitution, mass;
};

class b2World;

class Box2DSystem : public SystemInterface
{
public:
	Box2DSystem();
	void register_components(GGScene*) override;
	void update(GGScene*) override;
	void setting(GGScene*, const std::string&, std::variant<std::string, int, float>) override;
	void init_scripting(GGScene*) override;

	float factor() const { return scale_factor; }
	b2World* world() const { return m_world.get(); }

private:
	std::chrono::duration<float> step;
	std::unique_ptr<b2World> m_world;
	float scale_factor;

};

class Box2DComponent : public ComponentInterface
{
public:
	Box2DComponent(Box2DSystem* w) : sys(w) {}
	void add(GGScene*, entt::registry&, entt::entity, nlohmann::json&) override;
	bool create_instance(GGScene*, entt::registry&, entt::entity, nlohmann::json&) override;
	
private:
	Box2DSystem* sys;
};


