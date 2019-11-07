#include "pch.h"
#include "GlobalX.h"
#include "SoundItem.h"
//#include "ShaderProgram.h"

using namespace winrt;
using namespace winrt::Windows;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::ViewManagement;

void GlobalX::init(CoreWindow& win)
{
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	D3D_FEATURE_LEVEL mLevel;

	com_ptr<ID3D11Device> d3dDevice;
	com_ptr<ID3D11DeviceContext> d3dDeviceContext;
	HRESULT hok = D3D11CreateDevice(
		nullptr,                    // specify nullptr to use the default adapter
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,                    // leave as nullptr if hardware is used
		D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT,              // optionally set debug and Direct2D compatibility flags
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,          // always set this to D3D11_SDK_VERSION
		d3dDevice.put(),
		&mLevel,
		d3dDeviceContext.put()
	);

	d3dDevice.as(mdev);
	d3dDeviceContext.as(mcont);

	if (hok != S_OK)
	{
		OutputDebugString(L"FAILED TO CREATE DIRECT3D DEVICE!\n");
		return;
	}

	createSwapChain(win);

	create2d(win);

	D3D11_FEATURE_DATA_THREADING thread_support;
	mdev->CheckFeatureSupport(D3D11_FEATURE_THREADING, (void*)&thread_support, sizeof(thread_support));
	if (thread_support.DriverConcurrentCreates)
	{
		OutputDebugStringA("\nDriver supports Concurrent Creates\n");
	}
	else {
		OutputDebugStringA("\nDriver does not support concurrent creates\n");
	}

	//ShaderProgram::init();
	return;
}

void GlobalX::create2d(winrt::Windows::UI::Core::CoreWindow& win)
{
	D2D1_FACTORY_OPTIONS options = {};
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

	auto hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		options,
		mfact2d.put()
	);

	com_ptr<IDXGIDevice> dxgiDevice;
	mdev.as(dxgiDevice);

	hr = mfact2d->CreateDevice(dxgiDevice.get(), mdev2d.put());

	mdev2d->CreateDeviceContext(
		D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
		mcont2d.put()
	);

	// done creating device/context, now set backbuffer target

		// Now we set up the Direct2D render target bitmap linked to the swapchain. 
	// Whenever we render to this bitmap, it will be directly rendered to the 
	// swapchain associated with the window. ??????
	D2D1_BITMAP_PROPERTIES1 bitprops;
	ZeroMemory(&bitprops, sizeof(bitprops));
	bitprops.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	bitprops.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED };

	// Direct2D needs the dxgi version of the backbuffer surface pointer.
	com_ptr<IDXGISurface> dxgiBackBuffer;
	mswap->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));

	com_ptr<ID2D1Bitmap1> m_d2dTargetBitmap;

	// Get a D2D surface from the DXGI back buffer to use as the D2D render target.
	mcont2d->CreateBitmapFromDxgiSurface(
		dxgiBackBuffer.get(),
		bitprops,
		m_d2dTargetBitmap.put()
	);


	// So now we can set the Direct2D render target.
	mcont2d->SetTarget(m_d2dTargetBitmap.get());

	DWriteCreateFactory(DWRITE_FACTORY_TYPE::DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory), reinterpret_cast<::IUnknown**>(dwfactory.put()));

	return;
}


void GlobalX::createSwapChain(winrt::Windows::UI::Core::CoreWindow& win)
{


	// If the swap chain does not exist, create it.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

	swapChainDesc.Stereo = false;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.Flags = 0;

	// Use automatic sizing.
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;

	// This is the most common swap chain format.
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	// Don't use multi-sampling.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Use two buffers to enable flip effect.
	swapChainDesc.BufferCount = 2;

	// We recommend using this swap effect for all applications.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	// Once the swap chain description is configured, it must be
	// created on the same adapter as the existing D3D Device.

	// First, retrieve the underlying DXGI Device from the D3D Device.
	//ComPtr<IDXGIDevice2> dxgiDevice;
	//m_d3dDevice.As(&dxgiDevice);
	com_ptr<IDXGIDevice2> dxgiDevice = mdev.as<IDXGIDevice2>();

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces
	// latency and ensures that the application will only render after each VSync, minimizing
	// power consumption.
	dxgiDevice->SetMaximumFrameLatency(1);

	// Next, get the parent factory from the DXGI Device.
	com_ptr<IDXGIAdapter> dxgiAdapter;
	dxgiDevice->GetAdapter(dxgiAdapter.put());

	//com_ptr<IDXGIFactory2> dxgiFactory;
	//dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.put()));
	com_ptr<IDXGIFactory2> dxgiFactory;
	dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)dxgiFactory.put());

	com_ptr<::IUnknown> ik;
	win.as(ik);

	// Finally, create the swap chain.
	dxgiFactory->CreateSwapChainForCoreWindow(
		mdev.get(),
		ik.get(),
		&swapChainDesc,
		nullptr, // allow on all displays
		mswap.put()
	);

	// Once the swap chain is created, create a render target view.  This will
	// allow Direct3D to render graphics to the window.
	com_ptr<ID3D11Texture2D> backBuffer;
	mswap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.put());

	D3D11_RENDER_TARGET_VIEW_DESC vwdesc = { 0 };
	vwdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	vwdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	mdev->CreateRenderTargetView(
		backBuffer.get(),
		&vwdesc,
		mrentar.put()
	);

	// After the render target view is created, specify that the viewport,
	// which describes what portion of the window to draw to, should cover
	// the entire window.
	D3D11_TEXTURE2D_DESC backBufferDesc = { 0 };
	backBuffer->GetDesc(&backBufferDesc);

	D3D11_TEXTURE2D_DESC depthdesc;
	depthdesc.Width = backBufferDesc.Width;
	depthdesc.Height = backBufferDesc.Height;
	aspectRatio = backBufferDesc.Width / (float)backBufferDesc.Height;
	screen_width = backBufferDesc.Width;
	screen_height = backBufferDesc.Height;
	depthdesc.MipLevels = 1;
	depthdesc.ArraySize = 1;
	depthdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthdesc.SampleDesc.Count = 1;
	depthdesc.SampleDesc.Quality = 0;
	depthdesc.Usage = D3D11_USAGE_DEFAULT;
	depthdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthdesc.CPUAccessFlags = 0;
	depthdesc.MiscFlags = 0;
	mdev->CreateTexture2D(&depthdesc, nullptr, depthBuffer.put());

	D3D11_DEPTH_STENCIL_VIEW_DESC dviewd;
	ZeroMemory(&dviewd, sizeof(dviewd));
	dviewd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dviewd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dviewd.Texture2D.MipSlice = 0;
	mdev->CreateDepthStencilView(depthBuffer.get(), &dviewd, depthView.put());


	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(backBufferDesc.Width);
	viewport.Height = static_cast<float>(backBufferDesc.Height);
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;

	mcont->RSSetViewports(1, &viewport);

	return;
}

void Audio::init()
{
	XAudio2Create(audio2.put(), 0, XAUDIO2_DEFAULT_PROCESSOR);
	audio2->CreateMasteringVoice(&master_voice);

	WAVEFORMATEX wfx = { 0 };
	wfx.nAvgBytesPerSec = 44100 * 8;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 32;
	wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	wfx.nBlockAlign = 8;

	audio2->CreateSourceVoice(&main_music, (WAVEFORMATEX*)&wfx);
	main_music->Start();

	for (auto& cp : effects)
	{
		audio2->CreateSourceVoice(&cp, (WAVEFORMATEX*)&wfx);
		cp->Start();
	}
	return;
}

void Audio::switchMusic(SoundItem* mus)
{
	main_music->Stop();
	main_music->FlushSourceBuffers();

	if (!mus) return;

	XAUDIO2_BUFFER buf = { 0 };
	buf.AudioBytes = mus->data().size() * 4;
	buf.pAudioData = (BYTE*)mus->data().data();
	buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	main_music->SubmitSourceBuffer(&buf);
	main_music->Start();
	return;
}

int Audio::playEffect(SoundItem* eff)
{
	if (!eff) return -1;

	for (auto i = 0; i < effects.size(); ++i)
	{
		auto& cp = effects[i];
		XAUDIO2_VOICE_STATE state;
		cp->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

		if (state.BuffersQueued == 0)
		{
			XAUDIO2_BUFFER buf = { 0 };
			buf.AudioBytes = eff->data().size() * 4;
			buf.pAudioData = (BYTE*)eff->data().data();
			buf.Flags = XAUDIO2_END_OF_STREAM;
			cp->SubmitSourceBuffer(&buf);
			return i;
		}
	}

	return -1;
}

void Audio::setMusicVolume(float v)
{
	main_music->SetVolume(v);
	return;
}

void Audio::setEffectVolume(int index, float v)
{
	if (index < effects.size())
	{
		effects[index]->SetVolume(v);
	}

	return;
}

void Audio::mute(bool m)
{
	Audio::master_voice->SetVolume(m ? 1.0f : 0.0f);
	return;
}

winrt::com_ptr<IDXGISwapChain1> GlobalX::mswap;
winrt::com_ptr<ID3D11RenderTargetView> GlobalX::mrentar;
winrt::com_ptr<ID3D11Device1> GlobalX::mdev;
winrt::com_ptr<ID3D11DeviceContext1> GlobalX::mcont;
winrt::com_ptr<ID3D11Texture2D> GlobalX::depthBuffer;
winrt::com_ptr<ID3D11DepthStencilView> GlobalX::depthView;
float GlobalX::aspectRatio;
float GlobalX::screen_width;
float GlobalX::screen_height;

winrt::com_ptr<ID2D1Device> GlobalX::mdev2d;
winrt::com_ptr<ID2D1DeviceContext> GlobalX::mcont2d;
winrt::com_ptr<ID2D1Factory1> GlobalX::mfact2d;
winrt::com_ptr<IDWriteFactory> GlobalX::dwfactory;

winrt::com_ptr<IXAudio2> Audio::audio2;
IXAudio2MasteringVoice* Audio::master_voice;
IXAudio2SourceVoice* Audio::main_music;
std::vector<IXAudio2SourceVoice*> Audio::effects(8);
