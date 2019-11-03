#pragma once
#include "types.h"
#include "pch.h"

class Buffer
{
public:
	static Buffer* create(int size, D3D11_BIND_FLAG type, void* data = nullptr);
	static Buffer* create_static(int size, D3D11_BIND_FLAG type, void* data);

	ID3D11Buffer* get() const { return buf.get(); }

private:
	Buffer() : buf() {}
	winrt::com_ptr<ID3D11Buffer> buf;
};

