#pragma once
#include "GGScene.h"
#include <entt/entt.hpp>
#include "entt_extra.h"
#include <json.hpp>
#include <chrono>
#include <chipmunk/chipmunk.h>


class ChipmunkSystem : public SystemInterface
{
public:
	ChipmunkSystem();
	virtual void register_components(GGScene* s) override;
	virtual void update(GGScene*) override;
	virtual void init_scripting(GGScene*) override;

	std::chrono::duration<float> step;

	//virtual void setting(GGScene*, const std::string& stg, std::variant< std::string, int, float>) override;
	//virtual void begin(GGScene*) override;
	cpSpace* space() const { return world; }

private:
	GGScene* scene;
	cpSpace* world;
};

struct cpBody;

struct Phys2D
{
	cpBody* body;
};

class Physics2DComponent : public ComponentInterface
{
public:
	Physics2DComponent(ChipmunkSystem* cs) : chip(cs) {}

	//virtual void add(GGScene*, entt::registry&, entt::entity E, nlohmann::json& J) override;
	virtual bool create_instance(GGScene*, entt::registry& reg, entt::entity, nlohmann::json& J) override;
	
private:
	ChipmunkSystem* chip;
};
