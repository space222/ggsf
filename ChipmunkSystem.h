#pragma once
#include "GGScene.h"
#include <entt/entt.hpp>
#include <json.hpp>

struct cpSpace;

class ChipmunkSystem : public SystemInterface
{
public:
	ChipmunkSystem();
	virtual void register_components(GGScene* s) override;
	virtual void update(GGScene*) override;

	//virtual void setting(GGScene*, const std::string& stg, std::variant< std::string, int, float>) override;
	//virtual void begin(GGScene*) override;
	cpSpace* space() const { return world; }


private:
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

	//virtual void add(GGScene*, entt::entity E, nlohmann::json& J) override;
	virtual bool create_instance(GGScene*, entt::entity, nlohmann::json& J) override;
	
private:
	ChipmunkSystem* chip;
};
