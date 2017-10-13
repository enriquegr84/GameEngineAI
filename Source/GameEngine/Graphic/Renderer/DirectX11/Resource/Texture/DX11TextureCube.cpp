// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "Core/Logger/Logger.h"

#include "DX11TextureCube.h"

DX11TextureCube::DX11TextureCube(ID3D11Device* device, TextureCube const* textureCube)
    :
    DX11TextureArray(textureCube)
{
    // Specify the texture description.  TODO: Support texture cube RTs?
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = textureCube->GetLength();
    desc.Height = textureCube->GetLength();
    desc.MipLevels = textureCube->GetNumLevels();
    desc.ArraySize = textureCube->GetNumItems();
    desc.Format = static_cast<DXGI_FORMAT>(textureCube->GetFormat());
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    Resource::Usage usage = textureCube->GetUsage();
    if (usage == Resource::IMMUTABLE)
    {
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
    }
    else if (usage == Resource::DYNAMIC_UPDATE)
    {
        // DX11 does not allow a cube map to be a dynamic-update resource.
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
    }
    else  // usage == Resource::SHADER_OUTPUT
    {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
    }

    if (textureCube->IsAutogenerateMipmaps())
    {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_NONE;
        desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    // Create the texture.
    ID3D11Texture2D* dxTexture = nullptr;
    HRESULT hr;
    if (textureCube->GetData())
    {
        unsigned int const numSubresources =
            textureCube->GetNumSubresources();
        eastl::vector<D3D11_SUBRESOURCE_DATA> data(numSubresources);
        for (unsigned int index = 0; index < numSubresources; ++index)
        {
            auto sr = textureCube->GetSubresource(index);
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
		LogError("Failed to create texture, hr = " + GetErrorMessage(hr));
	}
	mDXObject = dxTexture;

    // Create views of the texture.
    CreateSRView(device, desc);
    if (textureCube->GetUsage() == Resource::SHADER_OUTPUT)
    {
        CreateUAView(device, desc);
    }

    // Create a staging texture if requested.
    if (textureCube->GetCopyType() != Resource::COPY_NONE)
    {
        CreateStaging(device, desc);
    }

    // Generate mipmaps if requested.
    if (textureCube->IsAutogenerateMipmaps() && mSRView)
    {
        ID3D11DeviceContext* context;
        device->GetImmediateContext(&context);
        context->GenerateMips(mSRView);
        context->Release();
    }
}

eastl::shared_ptr<GraphicObject> DX11TextureCube::Create(void* device, GraphicObject const* object)
{
    if (object->GetType() == GE_TEXTURE_CUBE)
    {
        return eastl::make_shared<DX11TextureCube>(
            reinterpret_cast<ID3D11Device*>(device),
            static_cast<TextureCube const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

void DX11TextureCube::CreateStaging(ID3D11Device* device, D3D11_TEXTURE2D_DESC const& tx)
{
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = tx.Width;
    desc.Height = tx.Height;
    desc.MipLevels = tx.MipLevels;
    desc.ArraySize = tx.ArraySize;
    desc.Format = tx.Format;
    desc.SampleDesc.Count = tx.SampleDesc.Count;
    desc.SampleDesc.Quality = tx.SampleDesc.Quality;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = D3D11_BIND_NONE;
    desc.CPUAccessFlags = msStagingAccess[GetTexture()->GetCopyType()];
    desc.MiscFlags = D3D11_RESOURCE_MISC_NONE;

    HRESULT hr = device->CreateTexture2D(&desc, nullptr,
        reinterpret_cast<ID3D11Texture2D**>(&mStaging));
	if (FAILED(hr))
	{
		LogError("Failed to create staging texture, hr = " + GetErrorMessage(hr));
	}
}

void DX11TextureCube::CreateSRView(ID3D11Device* device, D3D11_TEXTURE2D_DESC const& tx)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = tx.Format;
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    desc.TextureCube.MostDetailedMip = 0;
    desc.TextureCube.MipLevels = tx.MipLevels;
    HRESULT hr = device->CreateShaderResourceView(GetDXTexture(), &desc, &mSRView);
	if (FAILED(hr))
	{
		LogError("Failed to create shader resource view, hr = " + GetErrorMessage(hr));
	}
}

void DX11TextureCube::CreateUAView(ID3D11Device* device, D3D11_TEXTURE2D_DESC const& tx)
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    desc.Format = tx.Format;
    desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    desc.Texture2DArray.MipSlice = 0;
    desc.Texture2DArray.FirstArraySlice = 0;
    desc.Texture2DArray.ArraySize = tx.ArraySize;
    HRESULT hr = device->CreateUnorderedAccessView(GetDXTexture(), &desc, &mUAView);
	if (FAILED(hr))
	{
		LogError("Failed to create unordered access view, hr = " + GetErrorMessage(hr));
	}
}
