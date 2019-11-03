#include "pch.h"
#include <vector>
#include <string>
#include <future>
#include <thread>
#include <chrono>
#include <iostream>
#include <entt\entt.hpp>
#include <sol\sol.hpp>
#include "GlobalX.h"
#include "SoundItem.h"
#include "GGScene.h"

using namespace winrt;

using namespace Windows;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Composition;


void draw_load_screen(GGLoadProgress* progress)
{
	static float angle = 0;
	static bool init_complete = false;
	static com_ptr<IDWriteTextFormat> textFormat;
	static com_ptr<ID2D1SolidColorBrush> blue;
	static com_ptr<IDWriteTextLayout> circle;
	static DWRITE_TEXT_METRICS texmet;
	if (!init_complete)
	{
		init_complete = true;
		GlobalX::getDWFactory()->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 30, L"en-us", textFormat.put());
		GlobalX::getDWFactory()->CreateTextLayout(L"\u27F3", 1, textFormat.get(), 500, 500, circle.put());
		GlobalX::getContext2D()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightBlue), blue.put());
		circle->GetMetrics(&texmet);
	}

	float cc[] = { 0.0f, 0.0f, 0.0f, 0 };
	GlobalX::Clear(cc);
	GlobalX::getContext2D()->BeginDraw();
	GlobalX::getContext2D()->DrawText(L"Loading...", 10, textFormat.get(), D2D1_RECT_F{ 80, 900, 250, 1000 }, blue.get());

	float x = 220 + texmet.width / 2;

	GlobalX::getContext2D()->SetTransform(D2D1::Matrix3x2F::Rotation(angle, { x, 900 + texmet.height / 2 }));
	GlobalX::getContext2D()->DrawTextLayout({ 219,899 }, circle.get(), blue.get());

	GlobalX::getContext2D()->SetTransform(D2D1::Matrix3x2F::Identity());

	if (progress && progress->Expected)
	{
		std::wstring prog(winrt::to_hstring((int)(100.0f * (progress->Loaded / (float)progress->Expected))));
		//prog.resize(4);
		prog += L"%";
		GlobalX::getContext2D()->DrawText(prog.c_str(), prog.size(), textFormat.get(), D2D1_RECT_F{ 260, 900, 550, 1000 }, blue.get());
	}

	GlobalX::getContext2D()->EndDraw();
	GlobalX::Present(1, 0);

	angle += 2.0f;
	if (angle > 360) angle = 0;
	return;
}

template<typename T>
void wait_for_load(std::future<T>& asy, CoreWindow& win, GGLoadProgress* progress = nullptr)
{
	while (asy.valid() && !asy._Is_ready())
	{
		win.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
		draw_load_screen(progress);
	}
	return;
}


struct App : implements<App, IFrameworkViewSource, IFrameworkView>
{
	IFrameworkView CreateView() { return *this; }
	void Initialize(CoreApplicationView const&) {}
	void Load(hstring const&) {}
	void Uninitialize() {}

	void Run()
	{
		CoreWindow window = CoreWindow::GetForCurrentThread();
		window.Activate();

		std::string b = std::to_string(window.Bounds().Width);
		b += ", " + std::to_string(window.Bounds().Height) + " is the window size\n";
		OutputDebugStringA(b.c_str());

		ggsf_init();
		GlobalX::init(window);
		Audio::init();

		GGLoadProgress* GLP = new GGLoadProgress;
		GLP->Expected = 2171904; // 19170304;
		auto agn = std::async(GGScene::loadFile, L"tests.zip", GLP);

		CoreDispatcher dispatcher = window.Dispatcher();

		wait_for_load(agn, window, GLP);

		GGScene* scene = agn.get();

		GlobalX::Clear(scene->clearColor);
		GlobalX::Present(1, 0);

		std::string a = std::to_string(scene->load_progress->Loaded);
		a += " bytes loaded.\n";
		OutputDebugStringA(a.c_str());

		scene->begin();

		while (true)
		{
			dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			GlobalX::Clear(scene->clearColor);
			scene->render();
			GlobalX::Present(1, 0);
		}

		dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	}

	void SetWindow(CoreWindow const& window) {}
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	CoreApplication::Run(make<App>());
}
