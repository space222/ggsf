#include "pch.h"
#include "GGScene.h"
#include <json.hpp>
#include <vector>
#include <string>
#include <iostream>
#include "TagSystem.h"
#include "Render2DSystem.h"

using json = nlohmann::json;
std::unordered_map<std::string, system_factory_fn> ggsystem_factories;
void input_init(sol::state& lua);

GGScene* GGScene::loadFile(const std::wstring& fname, GGLoadProgress* LP)
{
	std::unique_ptr<FileSystem> zfs(ZipFileSystem::open_zip(fname));
	if (!zfs) return nullptr;

	std::vector<char> buf;
	{
		FileResource* ggsf = zfs->open("scene.ggsf");
		if (!ggsf) return nullptr;
		buf = std::move(ggsf->read_all());
	}

	json jpp;

	try {
		jpp = json::parse(std::begin(buf), std::end(buf));
	}
	catch (json::parse_error & e) {
		OutputDebugStringA("Scene parse error: ");
		OutputDebugStringA(e.what());
		OutputDebugStringA("\n");
		return nullptr;
	}

	if (!jpp.contains("settings"))
	{
		OutputDebugStringA("json settings object required\n");
		return nullptr;
	}

	json settings = jpp["settings"];
	GGScene* scene = new GGScene;
	scene->load_progress = LP;

	for (auto& ff : ggsystem_factories)
	{
		SystemInterface* SI = ff.second();
		SI->register_components(scene);
		scene->systems.insert(make_pair(ff.first, SI));
	}

	scene->fsys = zfs.release();

	input_init(scene->lua);

	if (settings.contains("script-file") && settings["script-file"].is_string())
	{
		std::string fname = settings["script-file"];
		std::vector<char> data;
		{
			std::unique_ptr<FileSystem> fs(scene->fsys->clone());
			std::unique_ptr<FileResource> fr(fs->open(fname));
			if( fr ) data = std::move(fr->read_all());
		}
		if (data.size())
		{
			std::string_view sv(data.data(), data.size());
			int success = 1;
			(void) scene->lua.safe_script(sv, [&](lua_State*, sol::protected_function_result pfr) 
				{ 
					success = 0;
					sol::error err = pfr;
					OutputDebugStringA("Lua error: ");
					OutputDebugStringA(err.what());
					OutputDebugStringA("\n");
					return pfr;
				});
			if (!success)
			{
				OutputDebugStringA("Fatal Error: Unable to parse script file.\n");
				delete scene;
				return nullptr;
			}
		}

		scene->lua.set_function("debug_print", OutputDebugStringA);
	}

	if (settings.contains("clear-color"))
	{
		const std::vector<float>& cc = jpp["settings"]["clear-color"].get<std::vector<float>>();
		scene->clearColor[0] = cc[0];
		scene->clearColor[1] = cc[1];
		scene->clearColor[2] = cc[2];
		scene->clearColor[3] = cc[3];
	}


	if (!settings.contains("systems"))
	{
		OutputDebugStringA("\n\nNO SYSTEMS FOUND\n\n");
		return scene;
	}

	{
		std::vector<std::string> systems = settings["systems"].get<std::vector<std::string>>();
		for (std::string& sys : systems)
		{
			if (scene->systems.find(sys) != scene->systems.end()) continue;
			auto factory_iter = ggsystem_factories.find(sys);
			if (factory_iter == std::end(ggsystem_factories))
			{
				std::string oa = "\n\nSYSTEM '" + sys + "' NOT REGISTERED\n\n";
				OutputDebugStringA(oa.c_str());
				continue;
			}
			SystemInterface* SI = factory_iter->second();
			scene->systems.insert(std::make_pair(sys, SI));
			SI->register_components(scene);
		}
	}

	if (settings.contains("system-settings"))
	{
		for (auto sts : settings["system-settings"].items())
		{
			std::string sysname = sts.key();
			auto iter = scene->systems.find(sysname);
			if (iter == std::end(scene->systems)) continue;
			SystemInterface* SI = iter->second;
			for (auto ssp : sts.value().items())
			{
				if (ssp.value().is_string())
					SI->setting(scene, ssp.key(), ssp.value().get<std::string>());
				if (ssp.value().is_number_integer())
					SI->setting(scene, ssp.key(), ssp.value().get<int>());
				if (ssp.value().is_number_float())
					SI->setting(scene, ssp.key(), ssp.value().get<float>());
			}
		}
	}
	
	if (settings.contains("begin-calls"))
	{
		std::vector<std::string> bcs = settings["begin-calls"].get<std::vector<std::string> >();
		for (const auto& sname : bcs)
		{
			auto iter = scene->systems.find(sname);
			if (iter == std::end(scene->systems)) continue; //todo: log error
			scene->begins.push_back(iter->second);
		}
	}
	else {
		//todo: just all of them in order
	}

	if (settings.contains("update-calls"))
	{
		std::vector<std::string> bcs = settings["update-calls"].get<std::vector<std::string> >();
		for (const auto& sname : bcs)
		{
			auto iter = scene->systems.find(sname);
			if (iter == std::end(scene->systems))
			{
				OutputDebugStringA("update system not found: ");
				OutputDebugStringA(sname.c_str());
				OutputDebugStringA("\n\n");
				continue; //todo: log error
			}
			scene->updates.push_back(iter->second);
		}
	}

	if (settings.contains("render-calls"))
	{
		std::vector<std::string> bcs = settings["render-calls"].get<std::vector<std::string> >();
		for (const auto& sname : bcs)
		{
			auto iter = scene->systems.find(sname);
			if (iter == std::end(scene->systems))
			{
				OutputDebugStringA("render system not found: ");
				OutputDebugStringA(sname.c_str());
				OutputDebugStringA("\n\n");
				continue; //todo: log error
			}
			scene->renders.push_back(iter->second);
		}
	}

	if (!jpp.contains("entities"))
	{
		OutputDebugStringA("\n\nNO ENTITIES FOUND!\n\n");
		return scene;
	}

	json entities = jpp["entities"];
	for (json& E : entities)
	{
		entt::entity ent = scene->entities.create();
		std::vector<std::pair<json, ComponentInterface*>> interfaces;
		
		for (auto& C : E.items())
		{
			std::string cname = C.key();
			if (cname == "Transform2D" || cname == "Transform3D") continue;
			auto iter = scene->components.find(cname);
			if (iter == scene->components.end()) continue; //todo: log error
			ComponentInterface* CI = iter->second;
			CI->add(scene, ent, C.value());
			interfaces.emplace_back(C.value(), CI);
		}

		if (E.contains("Transform2D"))
		{
			json trans = E["Transform2D"];
			if (trans.is_array())
			{
				const std::vector<float>& fdata = trans.get<std::vector<float> >();
				for (int i = 0; i < fdata.size() / 3; ++i)
				{
					entt::entity instance;
					if (i == 0)
						instance = ent;
					else
						instance = scene->entities.create(ent, scene->entities);

					scene->entities.assign_or_replace<Transform2D>(instance, fdata[i * 3], fdata[i * 3 + 1], fdata[i * 3 + 2]);

					for (auto P : interfaces)
					{
						P.second->create_instance(scene, instance, P.first);
					}
					
					//OutputDebugStringA("\n\nGOT TRANSFORMED!\n\n");
					if (i && scene->entities.has<EntityID>(instance))
					{
						std::string Id = scene->entities.get<EntityID>(instance).id;
						scene->entity_id.find(Id)->second.push_back(instance);
					}
				}
			} else {
				//todo: log error
			}
		}
	}

	while (scene->threads.load()) std::this_thread::sleep_for(std::chrono::milliseconds(30));

	return scene;
}

void GGScene::addLoadedBytes(int b)
{
	if (load_progress)
	{
		load_progress->Loaded += b;
	}
	return;
}

void GGScene::async(const std::function<void()>& func)
{
	if (this->threads.load() == 6)
	{
		func();
		return;
	}
	this->threads++;
	std::thread t(func);
	t.detach();
	return;
}

entt::entity GGScene::getEntity(const std::string& id, int index)
{
	auto iter = entity_id.find(id);
	if (iter == std::end(entity_id)) return entt::null;

	std::vector<entt::entity>& ets = iter->second;
	if (index >= ets.size()) return entt::null;

	return ets[index];
}

void GGScene::begin()
{
	sol::function bf = lua["begin"];
	if (bf.valid())
	{
		bf();
	}

	for (auto si : begins)
	{
		si->begin(this);
	}
	return;
}

void GGScene::update()
{
	sol::function uf = lua["update"];
	if (uf.valid())
	{
		uf();
	}

	for (auto si : updates)
	{
		si->update(this);
	}
	return;
}

void GGScene::render()
{
	for (auto si : renders)
	{
		si->render(this);
	}
	return;
}

std::unordered_map<std::string, system_factory_fn>& ggsf_get_system_factories()
{
	static std::unordered_map<std::string, system_factory_fn> factories;
	return factories;
}

void ggsf_init()
{
	ggsystem_factories = std::move(ggsf_get_system_factories());
	return;
}

void ggsf_register_system_factory(const std::string& name, system_factory_fn factory)
{
	ggsf_get_system_factories().insert(make_pair(name, factory));
	return;
}

