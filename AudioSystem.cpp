#include "pch.h"
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <entt/entt.hpp>
#include "entt_extra.h"
#include "AudioSystem.h"
#include "SoundItem.h"
#include "GlobalX.h"

void AudioSystem::register_components(GGScene* scene)
{
	scene->components.insert(std::make_pair("sound-item", new SoundComponent()));
	return;
}

SystemInterface* AudioSystem_factory()
{
	return new AudioSystem;
}

void AudioSystem::setting(GGScene*, const std::string& setting, std::variant<std::string, int, float> value)
{
	if (setting == "default-music" && std::holds_alternative<std::string>(value))
	{
		defaultMusic = std::get<std::string>(value);
	}
	return;
}

void AudioSystem::begin(GGScene* scene)
{
	if (defaultMusic.empty()) return;
	entt::entity E = scene->getEntity(defaultMusic);
	if (E == entt::null || !scene->entities.has<SoundSource>(E)) return;
	Audio::switchMusic(scene->entities.get<SoundSource>(E).item);
	return;
}

void SoundComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
{
	if (J.is_string())
	{
		std::string fname = J.get<std::string>();
		scene->resource_lock.lock();
		auto iter = scene->resources.insert(std::make_pair(fname, (SoundItem*)nullptr));
		if (!iter.second)
		{
			auto vari = iter.first->second;
			scene->resource_lock.unlock();
			if (std::holds_alternative<SoundItem*>(vari))
			{
				reg.assign<SoundSource>(E, std::get<SoundItem*>(vari));
			}
		}
		else {
			SoundItem* si = new SoundItem;
			iter.first->second = si;
			scene->resource_lock.unlock();
			entt::registry* R = &reg;
			scene->async([=] {
				bool res = si->loadFile(scene, fname);
				if (res)
				{
					R->assign<SoundSource>(E, si);
				} else {
					delete si;
				}
				scene->threads--;
				return;
				});
		}
	}
	return;
}

void AudioSystem::init_scripting(GGScene* scene)
{
	auto& lua = scene->lua;

	lua.set_function("playEffect", [=](GGEntity* ent) {
		if (!ent) return;
		SoundItem* SI = get_member_or(*ent->reg, ent->E, &SoundSource::item, (SoundItem*)nullptr);
		Audio::playEffect(SI);
		});
	return;
}

REGISTER_SYSTEM(Audio)


