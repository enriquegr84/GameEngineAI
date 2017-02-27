// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "Core/Logger/Logger.h"

#include "DX11VertexBuffer.h"

DX11VertexBuffer::DX11VertexBuffer(ID3D11Device* device, VertexBuffer const* vbuffer)
    :
    DX11Buffer(vbuffer)
{
    // Specify the buffer description.
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = vbuffer->GetNumBytes();
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.MiscFlags = D3D11_RESOURCE_MISC_NONE;
    desc.StructureByteStride = 0;
    Resource::Usage usage = vbuffer->GetUsage();
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
    else  // usage == Resource::SHADER_OUTPUT
    {
        // TODO: Write a sample application to test this case.
        LogError("Vertex output streams are not yet tested.");
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
    }

    // Create the buffer.
    ID3D11Buffer* buffer = nullptr;
    HRESULT hr;
    if (vbuffer->GetData())
    {
        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = vbuffer->GetData();
        data.SysMemPitch = 0;
        data.SysMemSlicePitch = 0;
        hr = device->CreateBuffer(&desc, &data, &buffer);
    }
    else
    {
        hr = device->CreateBuffer(&desc, nullptr, & buffer);
    }
	if (FAILED(hr))
	{
		LogError("Failed to create vertex buffer, hr = " + GetErrorMessage(hr));
	}
	mDXObject = buffer;

    // Create a staging buffer if requested.
    if (vbuffer->GetCopyType() != Resource::COPY_NONE)
    {
        CreateStaging(device, desc);
    }
}

eastl::shared_ptr<GraphicObject> DX11VertexBuffer::Create(
	void* device, GraphicObject const* object)
{
    if (object->GetType() == GE_VERTEX_BUFFER)
    {
        return eastl::make_shared<DX11VertexBuffer>(
			reinterpret_cast<ID3D11Device*>(device),
            static_cast<VertexBuffer const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

void DX11VertexBuffer::Enable(ID3D11DeviceContext* context)
{
	//	Binding the vertex buffer to the input-assembler stage which establishes
	//	the CPU-to-GPU vertex input stream
    if (mDXObject)
    {
        // The MSDN documentation for ID3D11DeviceContext::IASetVertexBuffers
        // and ID3D11DeviceContext::Draw(numVertices, startVertex) appears
        // not to mention that startVertex is relative to the offsets[]
        // passed to IASetVertexBuffers.  If you set the offsets[0] here, you
        // should call Draw(numVertices,0).  If you instead call
        // Draw(numVertices, startVertex), then you should set offsets[0]
        // to 0. The latter choice is made for GameEngine.
        ID3D11Buffer* buffers[1] = { GetDXBuffer() };
        VertexBuffer* vbuffer = GetVertexBuffer();
        UINT strides[1] = { vbuffer->GetElementSize() };
        UINT offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, buffers, strides, offsets);
    }
}

void DX11VertexBuffer::Disable(ID3D11DeviceContext* context)
{
    if (mDXObject)
    {
        ID3D11Buffer* buffers[1] = { nullptr };
        UINT strides[1] = { 0 };
        UINT offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, buffers, strides, offsets);
    }
}
