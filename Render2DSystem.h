#pragma once
#include "GGScene.h"
#include <glm/glm.hpp>

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

class Shader;
class Buffer;

class Render2DSystem : public SystemInterface
{
public:
	Render2DSystem();
	virtual void register_components(GGScene* scene) override;
	virtual void render(GGScene* scene) override;

	Shader* basic2d;
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