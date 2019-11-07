#pragma once
#include "GGScene.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <chrono>

struct Anim2D_data
{
	std::string tex_id;
	Texture* tex;
	GGRect first;
	int num_frames;
	int fps;
};

struct Anim2D
{
	Anim2D_data* anim;
};

struct Anim2D_ref
{
	std::string id;
	Anim2D_data* anim;
	int current_frame;
	std::chrono::system_clock::time_point stamp;
	bool playing;
};

struct Image2D
{
	Texture* tex;
};

struct ZIndex
{
	int z;
};

struct ImageRef
{
	std::string id;
	Texture* tex;
	bool crop;
	int x, y, width, height;
};

struct Geom2d
{
	int type;
	glm::vec4 extents;
	glm::vec4 color;
};

class Shader;
class Buffer;

class Render2DSystem : public SystemInterface
{
public:
	Render2DSystem();
	virtual void register_components(GGScene* scene) override;
	virtual void render(GGScene* scene) override;

	Shader* basic2d;
	Shader* geom2d_circle;
	Buffer* basic2d_verts;
	Buffer* basic2d_consts;
	glm::mat4 persp;
private:

};

class ImageComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::entity, nlohmann::json&) override;
};

class ImageRefComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::entity, nlohmann::json&) override;
};

class Geom2dComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::entity, nlohmann::json&) override;
};

class ZIndexComponent : public ComponentInterface
{
public:
	virtual void add(GGScene*, entt::entity, nlohmann::json&) override;
};

