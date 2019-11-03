#pragma once
#include "types.h"
#include "pch.h"

class GGScene;

class Texture
{
public:
	Texture() : tex(), view(), Width(0), Height(0) {}

	bool loadFile(GGScene*, const std::string&);
	void create(int width, int height, void* data, bool hasAlpha = false);

	ID3D11Texture2D* get() const { return tex.get(); }
	ID3D11ShaderResourceView* get_view()  const { return view.get(); }

	int width() const { return Width; }
	int height() const { return Height; }

private:
	int Width, Height;
	winrt::com_ptr<ID3D11Texture2D> tex;
	winrt::com_ptr<ID3D11ShaderResourceView> view;
};

