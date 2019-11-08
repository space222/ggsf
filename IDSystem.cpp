#include "pch.h"
#include "IDSystem.h"

void IDSystem::register_components(GGScene* scene)
{
	scene->components.insert(std::make_pair("id", new IDComponent()));
	return;
}

SystemInterface* IDSystem_factory()
{
	return new IDSystem;
}

void IDComponent::add(GGScene* scene, entt::entity E, nlohmann::json& J)
{
	std::string ID = J.get<std::string>();
	auto iter = scene->entity_id.find(ID);
	if (iter != std::end(scene->entity_id))
	{
		//todo: log error
		return;
	}
	scene->entity_id.insert(std::make_pair(ID, std::vector<entt::entity>{ E }));

	scene->entities.assign<EntityID>(E, ID);
	return;
}

REGISTER_SYSTEM(ID)


