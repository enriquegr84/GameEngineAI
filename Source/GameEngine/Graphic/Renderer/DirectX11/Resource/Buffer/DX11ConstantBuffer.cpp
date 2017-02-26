// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "Core/Logger/Logger.h"

#include "DX11ConstantBuffer.h"

DX11ConstantBuffer::DX11ConstantBuffer(ID3D11Device* device, ConstantBuffer const* cbuffer)
    :
    DX11Buffer(cbuffer)
{
    // Specify the buffer description.
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = cbuffer->GetNumBytes();
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.MiscFlags = D3D11_RESOURCE_MISC_NONE;
    desc.StructureByteStride = 0;
    Resource::Usage usage = cbuffer->GetUsage();
    if (usage == Resource::IMMUTABLE)
    {
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
    }
    else if (usage == Resource::DYNAMIC_UPDATE)
    {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
    }

    // Create the buffer.
    ID3D11Buffer* buffer = nullptr;
    HRESULT hr;
    if (cbuffer->GetData())
    {
        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = cbuffer->GetData();
        data.SysMemPitch = 0;
        data.SysMemSlicePitch = 0;
        hr = device->CreateBuffer(&desc, &data, &buffer);
    }
    else
    {
        hr = device->CreateBuffer(&desc, nullptr, &buffer);
    }
	if (FAILED(hr))
	{
		LogError("Failed to create constant buffer, hr = " + GetErrorMessage(hr));
	}
    mDXObject = buffer;

    // Create a staging buffer if requested.
    if (cbuffer->GetCopyType() != Resource::COPY_NONE)
    {
        CreateStaging(device, desc);
    }
}

eastl::shared_ptr<GraphicObject> DX11ConstantBuffer::Create(void* device, GraphicObject const* object)
{
    if (object->GetType() == GE_CONSTANT_BUFFER)
    {
        return eastl::make_shared<DX11ConstantBuffer>(
            reinterpret_cast<ID3D11Device*>(device),
            static_cast<ConstantBuffer const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}
