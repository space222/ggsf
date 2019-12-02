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
#include "IDSystem.h"
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
	glm::vec4 isize, crop;
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

Texture* resolve_img(GGScene* scene, Texture*& tex,const std::string& id)
{
	if (tex) return nullptr;
	entt::entity E = scene->getEntity(id);
	if (E == entt::null || !scene->entities.has<Image2D>(E) ) return nullptr;
	return tex = scene->entities.get<Image2D>(E).tex;
}


void Render2DSystem::render(GGScene* scene)
{
	std::vector<RJob> jobs;

	ID3D11SamplerState* samst = sampler.get();
	GlobalX::getContext()->PSSetSamplers(0, 1, &samst);

	float factor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GlobalX::getContext()->OMSetBlendState(blend.get(), factor, 0xffffffff);

	auto anims = scene->entities.view<Anim2D_ref, Transform2D>();
	for (entt::entity E : anims)
	{
		Anim2D_ref& ref = scene->entities.get<Anim2D_ref>(E);
//		if (!ref.playing) continue;
		if (ref.anim == nullptr)
		{
			entt::entity ad = scene->getEntity(ref.id);
			if (ad == entt::null || !scene->entities.has<Anim2D>(ad) ) continue;
			ref.anim = get_member_or(scene->entities, ad, &Anim2D::anim, (Anim2D_data*)nullptr);
			if (!ref.anim) continue;
		}
		if (ref.anim->tex == nullptr)
		{
			resolve_img(scene, ref.anim->tex, ref.anim->tex_id);
			if (!ref.anim->tex) continue;
		}

		ref.time += scene->lastFrameDuration();
		auto frame_span = std::chrono::milliseconds((int)(1000.0f / ref.anim->fps));
		if (ref.time >= frame_span)
		{
			ref.time -= frame_span;
			ref.current_frame++;
			if (ref.current_frame >= ref.anim->num_frames) ref.current_frame = 0;
		}

		int z = get_member_or(scene->entities, E, &ZIndex::z, 0);
		const Transform2D& trans = scene->entities.get<Transform2D>(E);

		float X = ref.anim->first.x + ref.anim->first.width * ref.current_frame;
		
		jobs.emplace_back(z, ref.anim->tex, trans.angle,
			glm::vec4(trans.x, trans.y, ref.anim->first.width, ref.anim->first.height),
			glm::vec4(X, ref.anim->first.y, ref.anim->first.width, ref.anim->first.height));
		jobs.back().type = 0;
	}

	auto view = scene->entities.view<ImageRef, Transform2D>();
	for (entt::entity E : view)
	{
		//if (!scene->entities.has<Transform2D>(E)) continue;
		Texture* tex = nullptr;

		ImageRef& ref = scene->entities.get<ImageRef>(E);
		if (ref.tex)
		{
			tex = ref.tex;
		} else {
			tex = resolve_img(scene, ref.tex, ref.id);
		}
		if (!tex) continue;

		const Transform2D trans = scene->entities.get<Transform2D>(E);
		const int zindex = get_member_or(scene->entities, E, &ZIndex::z, 0);
		//if (scene->entities.has<ZIndex>(E)) zindex = scene->entities.get<ZIndex>(E).z;
		
		if (ref.crop)
		{
			jobs.emplace_back(zindex, tex, trans.angle,
				glm::vec4(trans.x, trans.y, ref.width, ref.height),
				glm::vec4(ref.x, ref.y, ref.width, ref.height));
		} else {
			jobs.emplace_back(zindex, tex, trans.angle, 
				glm::vec4(trans.x, trans.y, tex->width(), tex->height()),
				glm::vec4(0.0f, 0.0f, tex->width(), tex->height()));
		}
		jobs.back().type = 0;
	}

	auto gview = scene->entities.view<Geom2d, Transform2D>();
	for (entt::entity E : gview)
	{
		const Transform2D trans = scene->entities.get<Transform2D>(E);
		const Geom2d geom = scene->entities.get<Geom2d>(E);
		const int zindex = get_member_or(scene->entities, E, &ZIndex::z, 0);
		
		jobs.emplace_back(zindex, nullptr, 0.0f,
			glm::vec4(trans.x /* - geom.extents.x */, trans.y /* - geom.extents.x */, geom.extents.x*2, geom.extents.x*2),
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
		cb->campos = glm::vec4(camera_pos.x,camera_pos.y,0,0);
		cb->recpos = job.isize;
		cb->crop = job.crop;
		cb->e1 = glm::vec4(job.angle * (3.14159f / 180.0f), (job.type > 0 ? 1 : 0), (job.tex ? job.tex->width() : job.isize.b), (job.tex ? job.tex->height() : job.isize.a));
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

	D3D11_SAMPLER_DESC sampdesc;
	ZeroMemory(&sampdesc, sizeof(sampdesc));
	sampdesc.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampdesc.MaxLOD = FLT_MAX;
	sampdesc.MinLOD = -FLT_MAX;
	sampdesc.MaxAnisotropy = 1;
	GlobalX::getDevice()->CreateSamplerState((D3D11_SAMPLER_DESC*)&sampdesc, sampler.put());

	D3D11_BLEND_DESC bldesc;
	ZeroMemory(&bldesc, sizeof(bldesc));
	bldesc.AlphaToCoverageEnable = true;
	bldesc.RenderTarget[0].BlendEnable = true;
	bldesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	bldesc.RenderTarget[0].SrcBlend = D3D11_BLEND::D3D11_BLEND_ONE;
	bldesc.RenderTarget[0].DestBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	bldesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	bldesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
	bldesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;
	bldesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	GlobalX::getDevice()->CreateBlendState(&bldesc, blend.put());

	return;
}

void Render2DSystem::register_components(GGScene* scene)
{
	scene->components.insert(std::make_pair("image", new ImageComponent()));
	scene->components.insert(std::make_pair("image-ref", new ImageRefComponent()));
	scene->components.insert(std::make_pair("animation2d", new Anim2dComponent()));
	scene->components.insert(std::make_pair("anim2d-ref", new Anim2dRefComponent()));

	scene->components.insert(std::make_pair("geometry2d", new Geom2dComponent()));

	scene->components.insert(std::make_pair("z-index", new ZIndexComponent()));
	return;
}

SystemInterface* Render2DSystem_factory()
{
	return new Render2DSystem;
}

void Anim2dComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
{
	if (!J.contains("image-id") || !J["image-id"].is_string() || !J.contains("crop")) return; //todo:log error

	Anim2D_data* ptr = new Anim2D_data;
	Anim2D_data& dat = *ptr;

	dat.tex_id = J["image-id"].get<std::string>();
	dat.tex = nullptr;

	std::vector<int> cr = J["crop"].get<std::vector<int>>();
	dat.first = GGRect{ cr[0], cr[1], cr[2], cr[3] };
	
	dat.fps = J["fps"].get<int>();
	dat.num_frames = J["num-frames"].get<int>();

	reg.assign<Anim2D>(E, ptr);
	return;
}

void Anim2dRefComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
{
	Anim2D_ref ref;

	ref.id = J["id"].get<std::string>();
	ref.current_frame = 0;
	ref.playing = true;
	ref.anim = nullptr;
	ref.time = std::chrono::milliseconds(0);

	reg.assign<Anim2D_ref>(E, ref);
	
	return;
}

void ImageRefComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
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

	reg.assign<ImageRef>(E, ImageRef{ id, nullptr, c, x, y, width, height });

	return;
}

void ImageComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
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
		reg.assign<Image2D>(E, text);
		scene->async([=] {
			text->loadFile(scene, fname);
			scene->threads--;
			return;
			});
	}

	return;
}

void Geom2dComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
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

	if( G.type ) reg.assign<Geom2d>(E, G);

	return;
}

void ZIndexComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
{
	int z = 0;
	if (J.is_number())
	{
		z = J.get<int>();
	}

	reg.assign<ZIndex>(E, z);

	return;
}

void r2d_set_image(GGEntity* ent, GGEntity* img)
{
	if (!ent || !img) return;

	if (img->reg->has<Image2D>(img->E))
	{
		if (ent->reg->has<Anim2D_ref>(ent->E))
			ent->reg->remove<Anim2D_ref>(ent->E);
		
		Image2D* I = img->reg->try_get<Image2D>(img->E);
		ImageRef IR;
		IR.crop = false;
		IR.width = I->tex->width();
		IR.height = I->tex->height();
		IR.tex = I->tex;
		ent->reg->assign_or_replace<ImageRef>(ent->E, IR);
		return;
	}

	if (img->reg->has<Anim2D>(img->E))
	{
		if (ent->reg->has<ImageRef>(ent->E))
			ent->reg->remove<ImageRef>(ent->E);

		Anim2D_data* data = img->reg->get<Anim2D>(img->E).anim;
		Anim2D_ref ref;
		ref.anim = data;
		ref.current_frame = 0;
		ref.playing = true;
		ref.time = std::chrono::duration<float>(0);
		ent->reg->assign_or_replace<Anim2D_ref>(ent->E, ref);
		return;
	}
	return;
}

void Render2DSystem::init_scripting(GGScene* scene)
{
	auto& lua = scene->lua;

	lua.create_table("Render2D")["setCamera"]
		= [=](const glm::vec2& pos) { this->camera_pos = pos; return; };

	lua.new_usertype<Transform2D>("Transform2D", sol::constructors<Transform2D(), Transform2D(float,float,float)>(),
						"x", &Transform2D::x, 
						"y", &Transform2D::y, 
						"angle", &Transform2D::angle);

	auto get_trans = [=](GGEntity* E) -> sol::object {
		if (!E) return sol::nil;
		Transform2D* t = E->reg->try_get<Transform2D>(E->E);
		if (!t) return sol::nil;
		return sol::make_object(scene->lua, *t);
	};

	auto set_trans = [=](GGEntity* ent, Transform2D* tran) {
		if (!ent || !tran) return;
		Transform2D* t = ent->reg->try_get<Transform2D>(ent->E);
		if (t) *t = *tran;
	};

	//lua["Entity"]["transform2d"] = sol::property(get_trans, set_trans);
	//^- results in a garbage address being passed to get_trans. no idea why.

	lua["Entity"]["getTransform2D"] = get_trans;
	lua["Entity"]["setTransform2D"] = set_trans;

	lua["Entity"]["setImage2D"] = r2d_set_image;

	return;
}


REGISTER_SYSTEM(Render2D)

