#pragma once
#include "GGScene.h"

struct EntityID
{
	std::string id;
};

class TagSystem : public SystemInterface
{
public:
	virtual void register_components(GGScene* scene) override;
};

class IDComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::entity, nlohmann::json&) override;
};

