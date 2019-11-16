#pragma once
#include "GGScene.h"
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_BUTTON_TRIGGER_ON_RELEASE 
#include <nuklear.h>

struct NuklearWidget
{
	int type;
	int x, y;
	int width, height;
	int data;
	std::string caption;
	std::string onClick;
};

struct NuklearWindow
{
	std::string title;
	std::vector<NuklearWidget> widgets;
};

class NuklearSystem : public SystemInterface
{
public:
	NuklearSystem();
	virtual void register_components(GGScene* scene) override;
	virtual void render(GGScene*) override;

	virtual void setting(GGScene*, const std::string&, std::variant<std::string, int, float>) override;

	nk_context ctx{};
	nk_user_font font;
	static winrt::com_ptr<IDWriteTextFormat> nk_text_format;
	static float font_height;
private:
	void draw_window(NuklearWindow&);
	void silo_launch(nk_context*);
	bool is_clipped = false;
};

class NuklearUIComponent : public ComponentInterface
{
public:
	NuklearUIComponent(NuklearSystem* sys) : system(sys) {}
	virtual void add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J) override;
private:
	NuklearSystem* system;
};

