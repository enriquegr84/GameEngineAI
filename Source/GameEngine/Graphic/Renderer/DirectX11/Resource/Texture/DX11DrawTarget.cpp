// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "DX11DrawTarget.h"

DX11DrawTarget::DX11DrawTarget(DrawTarget const* target,
    eastl::vector<DX11TextureRT*>& rtTextures, DX11TextureDS* dsTexture)
    :
	DrawTarget(),
	mTarget(const_cast<DrawTarget*>(target)),  // conceptual constness
    mRTTextures(rtTextures),
    mDSTexture(dsTexture),
    mRTViews(target->GetNumTargets()),
    mDSView(nullptr),
    mSaveRTViews(target->GetNumTargets()),
    mSaveDSView(nullptr)
{
    unsigned int const numTargets = mTarget->GetNumTargets();
    for (unsigned int i = 0; i < numTargets; ++i)
    {
        mRTViews[i] = mRTTextures[i]->GetRTView();
        mSaveRTViews[i] = nullptr;
    }

    if (mDSTexture)
    {
        mDSView = mDSTexture->GetDSView();
    }
}

eastl::shared_ptr<DrawTarget> DX11DrawTarget::Create(DrawTarget const* target,
	eastl::vector<GraphicObject*>& rtTextures, GraphicObject* dsTexture)
{
    eastl::vector<DX11TextureRT*> dxRTTextures(rtTextures.size());
    for (size_t i = 0; i < rtTextures.size(); ++i)
    {
        dxRTTextures[i] = static_cast<DX11TextureRT*>(rtTextures[i]);
    }
    DX11TextureDS* dxDSTexture = static_cast<DX11TextureDS*>(dsTexture);

    return eastl::make_shared<DX11DrawTarget>(target, dxRTTextures, dxDSTexture);
}

void DX11DrawTarget::Enable (ID3D11DeviceContext* context)
{
    UINT numViewports = 1;
    context->RSGetViewports(&numViewports, &mSaveViewport);

    UINT const numTargets = (UINT)mTarget->GetNumTargets();
    context->OMGetRenderTargets(numTargets, &mSaveRTViews[0], &mSaveDSView);

    D3D11_VIEWPORT viewport;
    viewport.Width = static_cast<float>(mTarget->GetWidth());
    viewport.Height = static_cast<float>(mTarget->GetHeight());
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    context->OMSetRenderTargets(numTargets, &mRTViews[0], mDSView);
}

void DX11DrawTarget::Disable (ID3D11DeviceContext* context)
{
    context->RSSetViewports(1, &mSaveViewport);

    UINT const numTargets = (UINT)mTarget->GetNumTargets();
    context->OMSetRenderTargets(numTargets, &mSaveRTViews[0], mSaveDSView);
    for (unsigned int i = 0; i < numTargets; ++i)
    {
        if (mSaveRTViews[i])
        {
            SafeRelease(mSaveRTViews[i]);
        }
    }
    if (mSaveDSView)
    {
        SafeRelease(mSaveDSView);
    }
}

