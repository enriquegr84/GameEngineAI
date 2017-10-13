// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "DX11TextureRT.h"

DX11TextureRT::~DX11TextureRT()
{
    FinalRelease(mRTView);
}

DX11TextureRT::DX11TextureRT(ID3D11Device* device, TextureRT const* texture)
    :
    DX11Texture2(texture),
    mRTView(nullptr)
{
    // Specify the texture description.
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = texture->GetWidth();
    desc.Height = texture->GetHeight();
    desc.MipLevels = texture->GetNumLevels();
    desc.ArraySize = 1;
    desc.Format = static_cast<DXGI_FORMAT>(texture->GetFormat());
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
    desc.MiscFlags = (texture->IsShared() ?
        D3D11_RESOURCE_MISC_SHARED : D3D11_RESOURCE_MISC_NONE);

    if (texture->GetUsage() == Resource::SHADER_OUTPUT)
    {
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    if (texture->IsAutogenerateMipmaps() && !texture->IsShared())
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    // Create the texture.
    ID3D11Texture2D* dxTexture = nullptr;
    HRESULT hr;
    if (texture->GetData())
    {
        unsigned int const numSubresources = texture->GetNumSubresources();
        eastl::vector<D3D11_SUBRESOURCE_DATA> data(numSubresources);
        for (unsigned int index = 0; index < numSubresources; ++index)
        {
            auto sr = texture->GetSubresource(index);
            data[index].pSysMem = sr.data;
            data[index].SysMemPitch = sr.rowPitch;
            data[index].SysMemSlicePitch = 0;
        }
        hr = device->CreateTexture2D(&desc, &data[0], &dxTexture);
    }
    else
    {
        hr = device->CreateTexture2D(&desc, nullptr, &dxTexture);
    }
	if (FAILED(hr))
	{
		LogError("Failed to map create texture, hr =" + GetErrorMessage(hr));
	}
    mDXObject = dxTexture;

    // Create views of the texture.
    CreateSRView(device, desc);
    CreateRTView(device, desc);
    if (texture->GetUsage() == Resource::SHADER_OUTPUT)
    {
        CreateUAView(device, desc);
    }

    // Create a staging texture if requested.
    if (texture->GetCopyType() != Resource::COPY_NONE)
    {
        CreateStaging(device, desc);
    }

    // Generate mipmaps if requested.
    if (texture->IsAutogenerateMipmaps() && mSRView)
    {
        ID3D11DeviceContext* context;
        device->GetImmediateContext(&context);
        context->GenerateMips(mSRView);
        context->Release();
    }
}

DX11TextureRT::DX11TextureRT(ID3D11Device* device, DX11TextureRT const* dxSharedTexture)
    :
    DX11Texture2(dxSharedTexture->GetTexture()),
    mRTView(nullptr)
{
    ID3D11Texture2D* dxShared = dxSharedTexture->CreateSharedDXObject(device);
    mDXObject = dxShared;
    D3D11_TEXTURE2D_DESC desc;
    dxShared->GetDesc(&desc);
    CreateRTView(device, desc);
}

eastl::shared_ptr<GraphicObject> DX11TextureRT::Create(void* device, GraphicObject const* object)
{
    if (object->GetType() == GE_TEXTURE_RT)
    {
        return eastl::make_shared<DX11TextureRT>(
            reinterpret_cast<ID3D11Device*>(device),
            static_cast<TextureRT const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

void DX11TextureRT::CreateRTView(ID3D11Device* device, D3D11_TEXTURE2D_DESC const& tx)
{
    D3D11_RENDER_TARGET_VIEW_DESC desc;
    desc.Format = tx.Format;
    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    HRESULT hr = device->CreateRenderTargetView(GetDXTexture(), &desc, &mRTView);
	if (FAILED(hr))
	{
		LogError("Failed to create render-target view, hr =" + GetErrorMessage(hr));
	}
}
