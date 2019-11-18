#include "pch.h"
#include "IDSystem.h"

void IDSystem::register_components(GGScene* scene)
{
	scene->components.insert(std::make_pair("id", new IDComponent()));
	scene->components.insert(std::make_pair("template-id", new TemplateIDComponent()));
	return;
}

SystemInterface* IDSystem_factory()
{
	return new IDSystem;
}

void IDComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
{
	std::string ID = J.get<std::string>();
	
	auto iter = scene->entity_id.find(ID);
	if (iter != std::end(scene->entity_id))
	{
		//todo: log error
		return;
	}
	scene->entity_id.insert(std::make_pair(ID, new std::vector<entt::entity>{ E }));

	scene->entities.assign<EntityID>(E, ID);
	return;
}

void TemplateIDComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
{
	std::string ID = J.get<std::string>();

	auto iter = scene->template_id.find(ID);
	if (iter != std::end(scene->template_id))
	{
		//todo: log error
		return;
	}

	auto& TI = scene->templates.get<GGTemplateInfo>(E);
	scene->template_id.insert(std::make_pair(ID, TI.templat));
	TI.id = ID;
	return;
}

REGISTER_SYSTEM(ID)


