#include "pch.h"
#include <vector>
#include <string>
#include <algorithm>
#include "Render2DSystem.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GlobalX.h"
#include "Texture.h"
#include "Shader.h"
#include "Buffer.h"
#include "TagSystem.h"
#include <entt/entt.hpp>
#include "entt_extra.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct RJob
{
	RJob() { }
	RJob(int Z, Texture* t, float ang, glm::vec4 imgsz, glm::vec4 crp)
			: z(Z), tex(t), 
			  isize(imgsz), crop(crp), angle(ang) {}
	int type;
	int z;
	Texture* tex;
	glm::vec4 isize;
	glm::vec4 crop;
	float angle;
};

#pragma pack(push, 1)
struct CBuffer
{
	glm::mat4 persp;
	glm::vec4 campos;
	glm::vec4 recpos, crop, e1;
};
#pragma pack(pop)

void Render2DSystem::render(GGScene* scene)
{
	std::vector<RJob> jobs;

	auto view = scene->entities.view<ImageRef>();
	for (entt::entity E : view)
	{
		if (!scene->entities.has<Transform2D>(E)) continue;
		Texture* tex = nullptr;

		ImageRef& ref = scene->entities.get<ImageRef>(E);
		if (ref.tex)
		{
			tex = ref.tex;
		} else {
			entt::entity img = scene->getEntity(ref.id);
			if (img == entt::null || !scene->entities.has<Image2D>(img))
			{
				continue;
			}
			ref.tex = tex = scene->entities.get<Image2D>(img).tex;
		}
		if (!tex) continue;

		const Transform2D trans = scene->entities.get<Transform2D>(E);
		const int zindex = get_member_or(scene->entities, E, &ZIndex::z, 0);
		//if (scene->entities.has<ZIndex>(E)) zindex = scene->entities.get<ZIndex>(E).z;
		
		if (ref.crop)
		{
			jobs.emplace_back(zindex, tex, trans.angle,
				glm::vec4(trans.x, trans.y, tex->width(), tex->height()),
				glm::vec4(ref.x, ref.y, ref.width, ref.height));
		} else {
			jobs.emplace_back(zindex, tex, trans.angle, 
				glm::vec4(trans.x, trans.y, tex->width(), tex->height()),
				glm::vec4(0.0f, 0.0f, tex->width(), tex->height()));
		}
		jobs.back().type = 0;
	}

	auto gview = scene->entities.view<Geom2d>();
	for (entt::entity E : gview)
	{
		if (!scene->entities.has<Transform2D>(E)) continue;

		const Transform2D trans = scene->entities.get<Transform2D>(E);
		const Geom2d geom = scene->entities.get<Geom2d>(E);
		const int zindex = get_member_or(scene->entities, E, &ZIndex::z, 0);
		//if (scene->entities.has<ZIndex>(E)) zindex = scene->entities.get<ZIndex>(E).z;

		jobs.emplace_back(zindex, nullptr, 0.0f,
			glm::vec4(trans.x - geom.extents.x, trans.y - geom.extents.x, geom.extents.x*2, geom.extents.x*2),
			geom.color);
		jobs.back().type = geom.type;
	}


	std::sort(std::begin(jobs), std::end(jobs), [&](auto& a, auto& b) {
		return a.z > b.z;
		});

	for (auto& job : jobs)
	{
		D3D11_MAPPED_SUBRESOURCE mapres;
		GlobalX::getContext()->Map(reinterpret_cast<ID3D11Resource*>(basic2d_consts->get()), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapres);
		CBuffer* cb = (CBuffer*)mapres.pData;
		cb->persp = persp;
		cb->campos = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		cb->recpos = job.isize;
		cb->crop = job.crop;
		cb->e1 = glm::vec4(job.angle * (3.14159f / 180.0f), 0.0f, (job.tex ? job.tex->width() : 0.0f), (job.tex ? job.tex->height() : 0.0f));
		GlobalX::getContext()->Unmap(reinterpret_cast<ID3D11Resource*>(basic2d_consts->get()), 0);
		
		ID3D11Buffer* c1 = basic2d_consts->get();
		GlobalX::BindTarget();

		if (job.type == 0)
		{
			basic2d->bind();
			ID3D11ShaderResourceView* v1 = job.tex->get_view();
			GlobalX::getContext()->VSSetConstantBuffers(0, 1, &c1);
			GlobalX::getContext()->PSSetShaderResources(0, 1, &v1);
		} else if (job.type == 1) {
			geom2d_circle->bind();
			GlobalX::getContext()->VSSetConstantBuffers(0, 1, &c1);
			GlobalX::getContext()->PSSetConstantBuffers(0, 1, &c1);
		}

		GlobalX::getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GlobalX::getContext()->Draw(6, 0);
	}

	return;
}

Render2DSystem::Render2DSystem()
{
	#include "basic2d_vert.h"
	#include "basic2d_pix.h"
	#include "geom2d_circle_pix.h"
	winrt::com_ptr<ID3D11VertexShader> vert_b2d;
	winrt::com_ptr<ID3D11PixelShader> pix_b2d;
	winrt::com_ptr<ID3D11InputLayout> layout_b2d;
	winrt::com_ptr<ID3D11PixelShader> g2dc;
	GlobalX::getDevice()->CreateVertexShader(basic2d_vert_bc, sizeof(basic2d_vert_bc), nullptr, vert_b2d.put());
	GlobalX::getDevice()->CreatePixelShader(basic2d_pix_bc, sizeof(basic2d_pix_bc),  nullptr, pix_b2d.put());
	GlobalX::getDevice()->CreatePixelShader(geom2d_circle_pix, sizeof(geom2d_circle_pix), nullptr, g2dc.put());

	//D3D11_INPUT_ELEMENT_DESC elemdesc[] = {
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0 },
	//};

	//GlobalX::getDevice()->CreateInputLayout(elemdesc, 1, basic2d_vert_bc, sizeof(basic2d_vert_bc), layout_b2d.put());

	basic2d = new Shader(vert_b2d, pix_b2d, layout_b2d);
	geom2d_circle = new Shader(vert_b2d, g2dc, layout_b2d);

	float verts[] = {
		0, 0, 0,0, 0,0, 0,0
	};
	basic2d_verts = Buffer::create_static(8 * 4, D3D11_BIND_VERTEX_BUFFER, (void*)verts);
	basic2d_consts = Buffer::create(128, D3D11_BIND_CONSTANT_BUFFER);

	persp = glm::ortho<float>(0.0f, GlobalX::screenWidth(), GlobalX::screenHeight(), 0.0f);

	return;
}

void Render2DSystem::register_components(GGScene* scene)
{
	scene->components.insert(std::make_pair("image", new ImageComponent()));
	scene->components.insert(std::make_pair("image-ref", new ImageRefComponent()));
	scene->components.insert(std::make_pair("geometry2d", new Geom2dComponent()));

	scene->components.insert(std::make_pair("z-index", new ZIndexComponent()));
	return;
}

SystemInterface* Render2DSystem_factory()
{
	return new Render2DSystem;
}

void ImageRefComponent::add(GGScene* scene, entt::entity E, nlohmann::json& J)
{
	std::string id;
	int x=0, y = 0, width = 0, height = 0;
	bool c = false;
	if (J.is_string())
	{
		id = J.get<std::string>();
	} else {
		if (J.contains("crop"))
		{
			std::vector<int> crop = J["crop"].get<std::vector<int> >();
			if (crop.size() != 4) return;
			x = crop[0]; y = crop[1]; width = crop[2]; height = crop[3];
			c = true;
		}
		if (!J.contains("id")) return;
		id = J["id"].get<std::string>();
	}

	scene->entities.assign<ImageRef>(E, ImageRef{ id, nullptr, c, x, y, width, height });

	return;
}

void ImageComponent::add(GGScene* scene, entt::entity E, nlohmann::json& J)
{
	if (!J.is_string()) return;
	std::string fname = J.get<std::string>();

	scene->resource_lock.lock();
	auto iter = scene->resources.insert(std::make_pair(fname, (Texture*)nullptr));
	if (!iter.second)
	{
		auto vari = iter.first->second;
		scene->resource_lock.unlock();
		if (std::holds_alternative<Texture*>(vari))
		{
			OutputDebugStringA("\nDidn't need a thread\n");
			scene->entities.assign<Image2D>(E, std::get<Texture*>(vari));
		}
		else {
			OutputDebugStringA("\nNOT THE RIGHT FILE TYPE\n");
		}
	}
	else {
		Texture* text = new Texture;
		iter.first->second = text;
		scene->resource_lock.unlock();
		scene->entities.assign<Image2D>(E, text);
		scene->async([=] {
			text->loadFile(scene, fname);
			scene->threads--;
			return;
			});
	}

	return;
}

void Geom2dComponent::add(GGScene* scene, entt::entity E, nlohmann::json& J)
{
	Geom2d G;
	G.type = 0;

	if (!J.contains("extents"))
	{
		return;
	}

	if (J.contains("shape") && J["shape"].is_string())
	{
		std::string sp = J["shape"].get<std::string>();
		if (sp == "circle")
		{
			G.type = 1;
		}
	}

	if (G.type == 1)
	{
		if ( !J["extents"].is_number() ) return;
		G.extents.x = J["extents"].get<float>();
	}

	if (J.contains("color") && J["color"].is_array())
	{
		std::vector<float> c = J["color"].get<std::vector<float>>();
		if (c.size() < 3) return;
		G.color = glm::vec4(c[0], c[1], c[2], 1);
		if (c.size() >= 4) G.color.a = c[3];
	}

	if( G.type ) scene->entities.assign<Geom2d>(E, G);

	return;
}

void ZIndexComponent::add(GGScene* scene, entt::entity E, nlohmann::json& J)
{
	int z = 0;
	if (J.is_number())
	{
		z = J.get<int>();
	}

	scene->entities.assign<ZIndex>(E, z);

	return;
}



REGISTER_SYSTEM(Render2D)

