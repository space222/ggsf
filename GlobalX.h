#pragma once
#include "pch.h"

class SoundItem;

class Audio
{
public:
	static void setMusicVolume(float);
	static void setEffectVolume(int, float);
	static void switchMusic(SoundItem*);
	static int playEffect(SoundItem*);
	static void mute(bool);

	static void init();
private:
	static winrt::com_ptr<IXAudio2> audio2;
	static IXAudio2MasteringVoice* master_voice;
	static IXAudio2SourceVoice* main_music;
	static std::vector<IXAudio2SourceVoice*> effects;
};

class GlobalX
{
public:
	static ID3D11Device1* getDevice() { return mdev.get(); }
	static ID3D11DeviceContext1* getContext() { return mcont.get(); }

	static ID2D1Device* getDevice2D() { return mdev2d.get(); }
	static ID2D1DeviceContext* getContext2D() { return mcont2d.get(); }
	static ID2D1Factory1* getFactory2D() { return mfact2d.get(); }
	static IDWriteFactory* getDWFactory() { return dwfactory.get(); }

	static void init(winrt::Windows::UI::Core::CoreWindow&);

	static void Clear(float* col)
	{
		mcont->ClearRenderTargetView(mrentar.get(), col);
		mcont->ClearDepthStencilView(depthView.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		return;
	}

	static void BindTarget()
	{
		ID3D11RenderTargetView* views = mrentar.get();
		mcont->OMSetRenderTargets(1, &views, depthView.get());
		return;
	}

	static void Present(int a, int b)
	{
		mswap->Present(a, b);
		return;
	}

	static float aspect() { return aspectRatio; }

private:
	static float aspectRatio;
	static void createSwapChain(winrt::Windows::UI::Core::CoreWindow&);
	static void create2d(winrt::Windows::UI::Core::CoreWindow&);

	static winrt::com_ptr<IDXGISwapChain1> mswap;
	static winrt::com_ptr<ID3D11RenderTargetView> mrentar;
	static winrt::com_ptr<ID3D11Device1> mdev;
	static winrt::com_ptr<ID3D11DeviceContext1> mcont;
	static winrt::com_ptr<ID3D11Texture2D> depthBuffer;
	static winrt::com_ptr<ID3D11DepthStencilView> depthView;

	static winrt::com_ptr<ID2D1Device> mdev2d;
	static winrt::com_ptr<ID2D1DeviceContext> mcont2d;
	static winrt::com_ptr<ID2D1Factory1> mfact2d;
	static winrt::com_ptr<IDWriteFactory> dwfactory;
};

