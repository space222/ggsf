#pragma once
#include "pch.h"
#include "GlobalX.h"

class Shader
{
public:
	Shader(winrt::com_ptr<ID3D11VertexShader> v, winrt::com_ptr<ID3D11PixelShader> p
		, winrt::com_ptr<ID3D11InputLayout> lay) : vert(v), pix(p), layout(lay) {}


	void bind()
	{
		GlobalX::getContext()->PSSetShader(pix.get(), nullptr, 0);
		GlobalX::getContext()->VSSetShader(vert.get(), nullptr, 0);
		GlobalX::getContext()->IASetInputLayout(layout.get());
		return;
	}

private:
	winrt::com_ptr<ID3D11VertexShader> vert;
	winrt::com_ptr<ID3D11PixelShader> pix;
	winrt::com_ptr<ID3D11InputLayout> layout;
};