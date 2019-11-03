#include "pch.h"
#include "GlobalX.h"
#include "Texture.h"
#include "GGScene.h"
#include "stb_image.h"

bool Texture::loadFile(GGScene* scene, const std::string& filename)
{
	std::unique_ptr<FileSystem> fs(scene->fsys->clone());
	std::unique_ptr<FileResource> fr(fs->open(filename));
	if (!fr) return false;

	std::vector<char> image = std::move(fr->read_all());
	int width, height, comp = 4;
	char* data = (char*)stbi_load_from_memory((const stbi_uc*)image.data(), image.size(), &width, &height, &comp, 4);
	if (!data)
	{
		OutputDebugStringA("\n\nIMAGE LOAD FAIL\n\n");
		return false;
	}
	create(width, height, data, comp > 3);
	scene->addLoadedBytes(width * height * 4);

	stbi_image_free(data);

	return true;
}

void Texture::create(int width, int height, void* data, bool /*hasAlpha*/)
{
	if (this->tex) return;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE;
	desc.Width = Width = width;
	desc.Height = Height = height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.MipLevels = 1;

	D3D11_SUBRESOURCE_DATA dat = { 0 };
	dat.pSysMem = data;
	dat.SysMemPitch = width * 4;

	//winrt::com_ptr<ID3D11Texture2D> texy;

	auto hr = GlobalX::getDevice()->CreateTexture2D(&desc, &dat, tex.put());

	if (!SUCCEEDED(hr))
	{
		OutputDebugStringA("\n\nTEXTURE CREATE FAIL\n\n");
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewdesc;
	ZeroMemory(&viewdesc, sizeof(viewdesc));
	viewdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	viewdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewdesc.Texture2D.MipLevels = 1;
	GlobalX::getDevice()->CreateShaderResourceView(reinterpret_cast<ID3D11Resource*>(tex.get()), &viewdesc, view.put());

	//this->tex = texy;
	return;
}

