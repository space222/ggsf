#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <variant>
#include <mutex>
#include <json.hpp>
#include <entt\entt.hpp>
#include <sol\sol.hpp>
#include "nczlib.h"

class Texture;
class SoundItem;
class SystemInterface;
class ComponentInterface;

struct GGLoadProgress
{
	std::atomic<int> Loaded{ 0 };
	int Expected{ 0 };
};

struct Transform2D
{
	Transform2D() : x(0), y(0), angle(0) {}
	Transform2D(float X, float Y, float A) : x(X), y(Y), angle(A) {}
	float x, y, angle;
};

struct GGRect
{
	GGRect(int X, int Y, int w, int h) : x(X), y(Y), width(w), height(h) {}
	GGRect() : x(0), y(0), width(0), height(0) {}
	int x, y, width, height;
};

class GGScene
{
public:
	static GGScene* loadFile(const std::wstring&, GGLoadProgress*);
	void addLoadedBytes(int b);

	void async(const std::function<void()>&);

	entt::entity getEntity(const std::string&, int index = 0);
	entt::registry entities;
	std::unordered_map<std::string, std::vector<entt::entity> > entity_id;

	std::mutex resource_lock;
	std::unordered_map<std::string, std::variant<SoundItem*, Texture*>> resources;
	std::unordered_map<std::string, SystemInterface*> systems;
	std::unordered_map<std::string, ComponentInterface*> components;

	std::vector<SystemInterface*> begins;
	std::vector<SystemInterface*> updates;
	std::vector<SystemInterface*> renders;

	sol::state lua;

	FileSystem* fsys;
	GGLoadProgress* load_progress;
	std::atomic<int> threads;

	void begin();
	void update();
	void render();

	float clearColor[4] = { 0 };
protected:
	GGScene() { }


};

class SystemInterface
{
public:
	virtual void register_components(GGScene*) = 0;
	virtual void begin(GGScene*) {};
	virtual void update(GGScene*) {};
	virtual void render(GGScene*) {};
	virtual void setting(GGScene*, const std::string&, std::variant<std::string, int, float>) {}
};

class ComponentInterface
{
public:
	virtual void add(GGScene*, entt::entity, nlohmann::json&) {}
	virtual bool create_instance(GGScene*, entt::entity, nlohmann::json&) { return false; }
};

using system_factory_fn = SystemInterface * (*)();

void ggsf_init();
void ggsf_register_system_factory(const std::string&, system_factory_fn);

#define REGISTER_SYSTEM(name) namespace name##gns { class name##registerer { \
			public: name##registerer() { ggsf_register_system_factory(#name, name##System_factory); }}; \
			name##registerer garbage11##name; }

