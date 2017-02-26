// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "DX11ComputeShader.h"

DX11ComputeShader::DX11ComputeShader(ID3D11Device* device, Shader const* shader)
    :
    DX11Shader(shader)
{
    eastl::vector<unsigned char> const& code = shader->GetCompiledCode();

    ID3D11ClassLinkage* linkage = nullptr;
    ID3D11ComputeShader* dxShader = nullptr;
    HRESULT hr = device->CreateComputeShader(&code[0], code.size(), linkage, &dxShader);
	if (FAILED(hr))
	{
		LogError("Failed to create compute shader, hr = " +
			GetErrorMessage(hr) + ".");
	}

    mDXObject = dxShader;
}

eastl::shared_ptr<GraphicObject> DX11ComputeShader::Create(
	void* device, GraphicObject const* object)
{
    if (object->GetType() == GE_COMPUTE_SHADER)
    {
        return eastl::make_shared<DX11ComputeShader>(
            reinterpret_cast<ID3D11Device*>(device),
            static_cast<Shader const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

void DX11ComputeShader::Enable(ID3D11DeviceContext* context)
{
    if (mDXObject)
    {
        ID3D11ClassInstance* instances[1] = { nullptr };
        UINT numInstances = 0;
        ID3D11ComputeShader* dxShader = static_cast<ID3D11ComputeShader*>(mDXObject);
        context->CSSetShader(dxShader, instances, numInstances);
    }
}

void DX11ComputeShader::Disable(ID3D11DeviceContext* context)
{
    if (mDXObject)
    {
        ID3D11ClassInstance* instances[1] = { nullptr };
        UINT numInstances = 0;
        ID3D11ComputeShader* dxShader = nullptr;
        context->CSSetShader(dxShader, instances, numInstances);
    }
}

void DX11ComputeShader::EnableCBuffer(ID3D11DeviceContext* context,
    unsigned int bindPoint, ID3D11Buffer* buffer)
{
    if (mDXObject)
    {
        ID3D11Buffer* buffers[1] = { buffer };
        context->CSSetConstantBuffers(bindPoint, 1, buffers);
    }
}

void DX11ComputeShader::DisableCBuffer(ID3D11DeviceContext* context,
    unsigned int bindPoint)
{
    if (mDXObject)
    {
        ID3D11Buffer* buffers[1] = { nullptr };
        context->CSSetConstantBuffers(bindPoint, 1, buffers);
    }
}

void DX11ComputeShader::EnableSRView(ID3D11DeviceContext* context,
    unsigned int bindPoint, ID3D11ShaderResourceView* srView)
{
    if (mDXObject)
    {
        ID3D11ShaderResourceView* views[1] = { srView };
        context->CSSetShaderResources(bindPoint, 1, views);
    }
}

void DX11ComputeShader::DisableSRView(ID3D11DeviceContext* context,
    unsigned int bindPoint)
{
    if (mDXObject)
    {
        ID3D11ShaderResourceView* views[1] = { nullptr };
        context->CSSetShaderResources(bindPoint, 1, views);
    }
}

void DX11ComputeShader::EnableUAView(ID3D11DeviceContext* context,
    unsigned int bindPoint, ID3D11UnorderedAccessView* uaView,
    unsigned int initialCount)
{
    if (mDXObject)
    {
        ID3D11UnorderedAccessView* views[1] = { uaView };
        unsigned int initialCounts[1] = { initialCount };
        context->CSSetUnorderedAccessViews(bindPoint, 1, views,
            initialCounts);
    }
}

void DX11ComputeShader::DisableUAView(ID3D11DeviceContext* context,
    unsigned int bindPoint)
{
    if (mDXObject)
    {
        ID3D11UnorderedAccessView* views[1] = { nullptr };
        unsigned int initialCounts[1] = { 0xFFFFFFFFu };
        context->CSSetUnorderedAccessViews(bindPoint, 1, views,
            initialCounts);
    }
}

void DX11ComputeShader::EnableSampler(ID3D11DeviceContext* context,
    unsigned int bindPoint, ID3D11SamplerState* state)
{
    if (mDXObject)
    {
        ID3D11SamplerState* states[1] = { state };
        context->CSSetSamplers(bindPoint, 1, states);
    }
}

void DX11ComputeShader::DisableSampler(ID3D11DeviceContext* context,
    unsigned int bindPoint)
{
    if (mDXObject)
    {
        ID3D11SamplerState* states[1] = { nullptr };
        context->CSSetSamplers(bindPoint, 1, states);
    }
}
