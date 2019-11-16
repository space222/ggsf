#include "pch.h"
#include "NuklearSystem.h"
#define NK_IMPLEMENTATION
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_BUTTON_TRIGGER_ON_RELEASE 
#include <nuklear.h>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <codecvt>
#include <vector>
#include <entt/entt.hpp>
#include "GGScene.h"
#include "GlobalX.h"

void NuklearSystem::setting(GGScene*, const std::string& setting, std::variant<std::string, int, float> value)
{

	return;
}

float silo_get_text_width(nk_handle handle, float, const char* text, int len)
{
	std::string from(text);
	std::wstring ftxt(from.begin(), from.end());
	winrt::com_ptr<IDWriteTextLayout> layout;
	GlobalX::getDWFactory()->CreateTextLayout(ftxt.c_str(), ftxt.size(), NuklearSystem::nk_text_format.get(), 1000, 400, layout.put());
	DWRITE_TEXT_METRICS metrics;
	layout->GetMetrics(&metrics);
	return metrics.widthIncludingTrailingWhitespace;
}

NuklearSystem::NuklearSystem()
{
	if (nk_text_format) return;

	GlobalX::getDWFactory()->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 30, L"en-us", nk_text_format.put());
	winrt::com_ptr<IDWriteTextLayout> layout;
	GlobalX::getDWFactory()->CreateTextLayout(L"A", 1, NuklearSystem::nk_text_format.get(), 1000, 400, layout.put());
	DWRITE_TEXT_METRICS metrics;
	layout->GetMetrics(&metrics);
	font_height = metrics.height;

	font.height = font_height;
	font.width = &silo_get_text_width;
	nk_init_default(&ctx, (nk_user_font*)&font);
	return;
}

void NuklearSystem::register_components(GGScene* scene)
{
	scene->components.insert(std::make_pair("nuklear-window", new NuklearUIComponent(this)));

	return;
}

void NuklearSystem::draw_window(NuklearWindow& nw)
{
	if (nk_begin(&ctx, nw.title.c_str(), nk_rect(50, 50, 320, 320),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE | NK_WINDOW_SCALABLE))
	{
		if (nw.widgets.size() > 0 && nw.widgets[0].type != 3)
		{
			int cols;
			auto iter = std::find_if(std::begin(nw.widgets) + 1, std::end(nw.widgets), [&](auto& a) { return a.type == 2; });
			if (iter == std::end(nw.widgets)) cols = nw.widgets.size() - 1;
			else cols = (std::end(nw.widgets) - iter) - 1;
			nk_layout_row_begin(&ctx, NK_STATIC, 30, cols);
		}
		for (auto iter = std::begin(nw.widgets); iter != std::end(nw.widgets); ++iter)
		{
			NuklearWidget& wid = *iter;
			switch (wid.type)
			{
			case 0: // label
				nk_layout_row_push(&ctx, wid.width);
				nk_label(&ctx, wid.caption.c_str(), NK_TEXT_LEFT);
				break;
			case 1: // button
				nk_layout_row_push(&ctx, wid.width);
				nk_button_label(&ctx, wid.caption.c_str());
				break;
			case 2: // new-row
				nk_layout_row_end(&ctx);
				int cols;
				auto next = std::find_if(iter + 1, std::end(nw.widgets), [&](auto& a) { return a.type == 2; });
				if (next == std::end(nw.widgets)) cols = (std::end(nw.widgets) - iter) - 1;
				else cols = (next - iter) - 1;
				//OutputDebugStringA("columns: ");
				//OutputDebugStringA(std::to_string(cols).c_str());
				//OutputDebugStringA("\n\n");
				nk_layout_row_begin(&ctx, NK_STATIC, 30, cols);
				break;
			}

		}
	}
	nk_layout_row_end(&ctx);
	nk_end(&ctx);
	return;
}

void NuklearSystem::render(GGScene* scene)
{
	auto v = scene->entities.view<NuklearWindow>();
	
	for (auto& entity : v)
	{
		NuklearWindow& nw = scene->entities.get<NuklearWindow>(entity);
		draw_window(nw);
	}

	GlobalX::getContext2D()->BeginDraw();
	silo_launch(&ctx);
	if (is_clipped)
	{
		GlobalX::getContext2D()->PopAxisAlignedClip();
		is_clipped = false;
	}
	GlobalX::getContext2D()->EndDraw();
	nk_clear(&ctx);
	return;
}

SystemInterface* NuklearSystem_factory()
{
	return new NuklearSystem;
}

void NuklearUIComponent::add(GGScene* scene, entt::registry& reg, entt::entity E, nlohmann::json& J)
{
	NuklearWindow win;

	if (J.contains("title"))
	{
		win.title = J["title"].get<std::string>();
	}

	if (!J.contains("widgets"))
	{
		OutputDebugStringA("\n\nMAKES NO SENSE\n\n");
		return;
	}

	nlohmann::json widgets = J["widgets"];

	for (auto& jj : widgets)
	{
		NuklearWidget w;
		w.data = 0;

		if (jj.contains("x"))
		{
			w.x = jj["x"].get<int>();
		}
		if (jj.contains("y"))
		{
			w.y = jj["y"].get<int>();
		}
		if (jj.contains("width"))
		{
			w.width = jj["width"].get<int>();
		}
		if (jj.contains("height"))
		{
			w.height = jj["height"].get<int>();
		}
		if (jj.contains("title"))
		{
			w.caption = jj["title"].get<std::string>();
		}
		if (jj.contains("type"))
		{
			std::string tp = jj["type"].get<std::string>();
			if (tp == "label") w.type = 0;
			else if (tp == "button") w.type = 1;
			else if (tp == "new-row") w.type = 2;
		}

		win.widgets.push_back(w);
	}
	
	scene->entities.assign<NuklearWindow>(E, win);
	return;
}

void NuklearSystem::silo_launch(nk_context* nk)
{
	const struct nk_command* cmd = 0;
	nk_foreach(cmd, nk) {
		switch (cmd->type) {
		default: break; // printf("Unhandled cmd: %i\nexiting...", cmd->type); exit(1);
		case NK_COMMAND_NOP: break;
		case NK_COMMAND_CURVE:
		{
			nk_command_curve* cl = (nk_command_curve*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cl->color.r / 255.0f, cl->color.g / 255.0f, cl->color.b / 255.0f, 1.0f }, brush.put());

			//cairo_set_line_width(ct, cl->line_thickness);
			//cairo_move_to(ct, cl->begin.x, cl->begin.y);
			//cairo_curve_to(ct, cl->ctrl[0].x, cl->ctrl[0].y, cl->ctrl[1].x, cl->ctrl[1].y, cl->end.x, cl->end.y);
			//cairo_stroke(ct);
		}break;
		case NK_COMMAND_RECT_MULTI_COLOR:
		{
			nk_command_rect_multi_color* cl = (nk_command_rect_multi_color*)cmd;
			//todo: no idea how to do with with Direct2D

			/*cairo_pattern_t* pattern = cairo_pattern_create_mesh();
			cairo_mesh_pattern_begin_patch(pattern);
			cairo_mesh_pattern_move_to(pattern, cl->x, cl->y + cl->h);
			cairo_mesh_pattern_line_to(pattern, cl->x, cl->y);
			cairo_mesh_pattern_line_to(pattern, cl->x + cl->w, cl->y);
			cairo_mesh_pattern_line_to(pattern, cl->x + cl->w, cl->y + cl->h);
			//cairo_mesh_pattern_line_to(pattern, cl->x, cl->y+cl->h);

			cairo_mesh_pattern_set_corner_color_rgba(pattern, 3,
				cl->right.r / 255.0f, cl->right.g / 255.0f, cl->right.b / 255.0f, cl->right.a / 255.0f);
			cairo_mesh_pattern_set_corner_color_rgba(pattern, 2,
				cl->top.r / 255.0f, cl->top.g / 255.0f, cl->top.b / 255.0f, cl->top.a / 255.0f);
			cairo_mesh_pattern_set_corner_color_rgba(pattern, 1,
				cl->left.r / 255.0f, cl->left.g / 255.0f, cl->left.b / 255.0f, cl->left.a / 255.0f);
			cairo_mesh_pattern_set_corner_color_rgba(pattern, 0,
				cl->bottom.r / 255.0f, cl->bottom.g / 255.0f, cl->bottom.b / 255.0f, cl->bottom.a / 255.0f);
			cairo_mesh_pattern_end_patch(pattern);

			cairo_set_source(ct, pattern);
			cairo_rectangle(ct, cl->x, cl->y, cl->w, cl->h);
			cairo_fill(ct);
			cairo_pattern_destroy(pattern);
			*/
		}break;
		case NK_COMMAND_ARC_FILLED:
		{
			nk_command_arc_filled* cl = (nk_command_arc_filled*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cl->color.r / 255.0f, cl->color.g / 255.0f, cl->color.b / 255.0f, 1.0f }, brush.put());

			//cairo_set_source_rgb(ct, cl->color.r / 255.0f, cl->color.g / 255.0f, cl->color.b / 255.0f);
			//cairo_move_to(ct, cl->cx, cl->cy);
			//cairo_arc(ct, cl->cx, cl->cy, cl->r, cl->a[0], cl->a[1]);
			//cairo_fill(ct);
		}break;
		case NK_COMMAND_ARC:
		{
			nk_command_arc* cl = (nk_command_arc*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cl->color.r / 255.0f, cl->color.g / 255.0f, cl->color.b / 255.0f, 1.0f }, brush.put());

			//cairo_set_line_width(ct, cl->line_thickness);
			//cairo_move_to(ct, cl->cx, cl->cy);
			//cairo_arc(ct, cl->cx, cl->cy, cl->r, cl->a[0], cl->a[1]);
			//cairo_stroke(ct);
		}break;
		case NK_COMMAND_CIRCLE:
		{
			nk_command_circle* cl = (nk_command_circle*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cl->color.r / 255.0f, cl->color.g / 255.0f, cl->color.b / 255.0f, 1.0f }, brush.put());
			D2D1_ELLIPSE ellipse;
			ellipse.point = { (float)cl->x, (float)cl->y };
			ellipse.radiusX = (float)cl->w / 2;
			ellipse.radiusY = (float)cl->w / 2;
			GlobalX::getContext2D()->DrawEllipse(ellipse, brush.get(), cl->line_thickness);
		}break;
		case NK_COMMAND_CIRCLE_FILLED:
		{
			nk_command_circle_filled* cl = (nk_command_circle_filled*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cl->color.r / 255.0f, cl->color.g / 255.0f, cl->color.b / 255.0f, 1.0f }, brush.put());
			D2D1_ELLIPSE ellipse;
			ellipse.point = { (float)cl->x, (float)cl->y };
			ellipse.radiusX = (float)cl->w / 2;
			ellipse.radiusY = (float)cl->w / 2;
			GlobalX::getContext2D()->FillEllipse(ellipse, brush.get());
		}break;
		case NK_COMMAND_TEXT:
		{
			nk_command_text* txt = (nk_command_text*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> bg_brush;
			winrt::com_ptr<ID2D1SolidColorBrush> fr_brush;

			GlobalX::getContext2D()->CreateSolidColorBrush({ txt->background.r / 255.0f, txt->background.g / 255.0f, txt->background.b / 255.0f, 1.0f }, bg_brush.put());
			GlobalX::getContext2D()->CreateSolidColorBrush({ txt->foreground.r / 255.0f, txt->foreground.g / 255.0f, txt->foreground.b / 255.0f, 1.0f }, fr_brush.put());
			GlobalX::getContext2D()->FillRectangle({ (float)txt->x,(float)txt->y,(float)txt->w + txt->x,(float)txt->h + txt->y }, bg_brush.get());
			std::string from = txt->string;
			std::wstring w(from.begin(), from.end());
			GlobalX::getContext2D()->DrawText(w.c_str(), w.size(), nk_text_format.get(), { (float)txt->x, (float)txt->y, (float)txt->x + txt->w, (float)txt->y + txt->h }, fr_brush.get());

		}break;
		case NK_COMMAND_SCISSOR:
		{
			nk_command_scissor* sc = (nk_command_scissor*)cmd;
			if (this->is_clipped)
			{
				GlobalX::getContext2D()->PopAxisAlignedClip();
			}
			this->is_clipped = true;
			GlobalX::getContext2D()->PushAxisAlignedClip(
				D2D1_RECT_F{ (float)sc->x, (float)sc->y, (float)sc->x + sc->w, (float)sc->y + sc->h }, D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		}break;
		case NK_COMMAND_LINE:
		{
			nk_command_line* cl = (nk_command_line*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cl->color.r / 255.0f, cl->color.g / 255.0f, cl->color.b / 255.0f, 1.0f }, brush.put());

			GlobalX::getContext2D()->DrawLine({ (float)cl->begin.x, (float)cl->begin.y },
				{ (float)cl->end.x, (float)cl->end.y }, brush.get(), cl->line_thickness);

		}break;
		case NK_COMMAND_RECT:
		{
			nk_command_rect* cr = (nk_command_rect*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cr->color.r / 255.0f, cr->color.g / 255.0f, cr->color.b / 255.0f, 1.0f }, brush.put());

			GlobalX::getContext2D()->DrawRectangle(D2D1_RECT_F{ (float)cr->x,(float)cr->y,
				(float)cr->x + cr->w,(float)cr->y + cr->h },
				brush.get(), cr->line_thickness);
		}break;
		case NK_COMMAND_RECT_FILLED:
		{
			nk_command_rect_filled* cr = (nk_command_rect_filled*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cr->color.r / 255.0f, cr->color.g / 255.0f, cr->color.b / 255.0f, 1.0f }, brush.put());

			GlobalX::getContext2D()->FillRectangle(D2D1_RECT_F{ (float)cr->x
				,(float)cr->y,(float)cr->x + cr->w,(float)cr->y + cr->h },
				brush.get());
		}break;
		case NK_COMMAND_TRIANGLE:
		{
			nk_command_triangle* cr = (nk_command_triangle*)cmd;
			//cairo_set_source_rgb(ct, cr->color.r / 255.0f, cr->color.g / 255.0f, cr->color.b / 255.0f);
			//cairo_set_line_width(ct, cr->line_thickness);
			//cairo_move_to(ct, cr->a.x, cr->a.y);
			//cairo_line_to(ct, cr->b.x, cr->b.y);
			//cairo_line_to(ct, cr->c.x, cr->c.y);
			//cairo_line_to(ct, cr->a.x, cr->a.y);
			//cairo_stroke(ct);
		}break;
		case NK_COMMAND_TRIANGLE_FILLED:
		{
			nk_command_triangle_filled* cr = (nk_command_triangle_filled*)cmd;
			//cairo_set_source_rgb(ct, cr->color.r / 255.0f, cr->color.g / 255.0f, cr->color.b / 255.0f);
			//cairo_move_to(ct, cr->a.x, cr->a.y);
			//cairo_line_to(ct, cr->b.x, cr->b.y);
			//cairo_line_to(ct, cr->c.x, cr->c.y);
			//cairo_line_to(ct, cr->a.x, cr->a.y);
			//cairo_fill(ct);
		}break;
		case NK_COMMAND_POLYGON_FILLED:
		{
			nk_command_polygon_filled* cr = (nk_command_polygon_filled*)cmd;
			//cairo_set_source_rgb(ct, cr->color.r / 255.0f, cr->color.g / 255.0f, cr->color.b / 255.0f);
			//struct nk_vec2i* pts = (struct nk_vec2i*) & cr->points[0];
			//cairo_move_to(ct, pts[0].x, pts[0].y);
			//for (int ptindex = 1; ptindex < cr->point_count; ++ptindex) cairo_line_to(ct, pts[ptindex].x, pts[ptindex].y);
			//cairo_line_to(ct, pts[0].x, pts[0].y);
			//cairo_fill(ct);
		}break;
		case NK_COMMAND_POLYGON:
		{
			nk_command_polygon* cr = (nk_command_polygon*)cmd;
			struct nk_vec2i* pts = (struct nk_vec2i*) & cr->points[0];
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cr->color.r / 255.0f, cr->color.g / 255.0f, cr->color.b / 255.0f, 1.0f }, brush.put());

			//struct nk_vec2i* pts = (struct nk_vec2i*) & cr->points[0];

			for (int ptindex = 1; ptindex < cr->point_count; ++ptindex)
			{
				GlobalX::getContext2D()->DrawLine({ (float)pts[ptindex - 1].x, (float)pts[ptindex - 1].y },
					{ (float)pts[ptindex].x, (float)pts[ptindex].y }, brush.get(), cr->line_thickness);
			}

			GlobalX::getContext2D()->DrawLine({ (float)pts[0].x, (float)pts[0].y },
				{ (float)pts[cr->point_count - 1].x, (float)pts[cr->point_count].y },
				brush.get(), cr->line_thickness);
		}break;
		case NK_COMMAND_POLYLINE:
		{
			nk_command_polyline* cr = (nk_command_polyline*)cmd;
			winrt::com_ptr<ID2D1SolidColorBrush> brush;
			GlobalX::getContext2D()->CreateSolidColorBrush(D2D1_COLOR_F{
			   cr->color.r / 255.0f, cr->color.g / 255.0f, cr->color.b / 255.0f, 1.0f }, brush.put());

			struct nk_vec2i* pts = (struct nk_vec2i*) & cr->points[0];

			for (int ptindex = 1; ptindex < cr->point_count; ++ptindex)
			{
				GlobalX::getContext2D()->DrawLine({ (float)pts[ptindex - 1].x, (float)pts[ptindex - 1].y },
					{ (float)pts[ptindex].x, (float)pts[ptindex].y }, brush.get(), cr->line_thickness);
			}

			//cairo_line_to(ct, pts[ptindex].x, pts[ptindex].y);
			//cairo_line_to(ct, pts[0].x, pts[0].y); // looks like polyline doesn't connect, otherwise would be same
								 // as stroked polygon
			//cairo_stroke(ct);
		}break;
		} //end of cmd switch
	} //end of nk_foreach

	return;
}

REGISTER_SYSTEM(Nuklear)

winrt::com_ptr<IDWriteTextFormat> NuklearSystem::nk_text_format = nullptr;
float NuklearSystem::font_height;
