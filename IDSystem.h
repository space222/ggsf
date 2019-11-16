#pragma once
#include "GGScene.h"

struct EntityID
{
	std::string id;
};

struct TemplateID
{
	std::string id;
};

class IDSystem : public SystemInterface
{
public:
	virtual void register_components(GGScene* scene) override;
};

class IDComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::registry&, entt::entity, nlohmann::json&) override;
};

class TemplateIDComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::registry&, entt::entity, nlohmann::json&) override;
};
