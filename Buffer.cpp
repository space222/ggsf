#include "pch.h"
#include "GlobalX.h"
#include "Buffer.h"

Buffer* Buffer::create(int size, D3D11_BIND_FLAG type, void* data)
{
	winrt::com_ptr<ID3D11Buffer> buf;

	D3D11_BUFFER_DESC desc = { 0 };
	desc.BindFlags = type;
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA dat = { 0 };
	dat.pSysMem = data;

	GlobalX::getDevice()->CreateBuffer(&desc, (data ? &dat : nullptr), buf.put());

	Buffer* buffer = new Buffer;
	buffer->buf = buf;
	return buffer;
}

Buffer* Buffer::create_static(int size, D3D11_BIND_FLAG type, void* data)
{
	winrt::com_ptr<ID3D11Buffer> buf;

	D3D11_BUFFER_DESC desc = { 0 };
	desc.BindFlags = type;
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DEFAULT;
	
	D3D11_SUBRESOURCE_DATA dat = { 0 };
	dat.pSysMem = data;

	GlobalX::getDevice()->CreateBuffer(&desc, &dat, buf.put());

	Buffer* buffer = new Buffer;
	buffer->buf = buf;
	return buffer;
}