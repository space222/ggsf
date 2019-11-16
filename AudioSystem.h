#pragma once
#include "GGScene.h"
#include <entt/entt.hpp>
#include <json.hpp>

class SoundItem;

struct SoundSource
{
	SoundItem* item;
};

class AudioSystem : public SystemInterface
{
public:
	virtual void register_components(GGScene* s) override;
	virtual void setting(GGScene*, const std::string& stg, std::variant< std::string, int, float>) override;
	virtual void begin(GGScene*) override;
	virtual void init_scripting(GGScene*) override;
	std::string defaultMusic;
};

class SoundComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::registry&, entt::entity E, nlohmann::json& J) override;
	//virtual bool create_instance(GGScene*, entt::entity, nlohmann::json& J) override;
};

