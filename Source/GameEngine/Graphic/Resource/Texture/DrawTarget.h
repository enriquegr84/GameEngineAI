// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef DRAWTARGET_H
#define DRAWTARGET_H

#include "TextureDS.h"
#include "TextureRT.h"
#include <memory>

class GRAPHIC_ITEM DrawTarget
{
public:
    // Construction and destruction.  The number of supported targets depends
    // on the graphics hardware and drivers.  The number of render targets
    // must be at least 1.  Set 'hasRTData' to 'true' iff you want system
    // memory data for the color textures.  Set 'dsFormat' to a valid format
    // when you want depth-stencil; these are
    //   DF_D32_FLOAT_S8X24_UINT
    //   DF_D32_FLOAT
    //   DF_D24_UNORM_S8_UINT
    //   DF_D16_UNORM
    // Set 'dsFormat' to DF_UNKNOWN when you do not want a depth-stencil.  Set
    // 'hasDSData' to 'true' iff you want system memory data for the
    // depth-stencil.
    virtual ~DrawTarget();
	DrawTarget() {}
    DrawTarget(unsigned int numRenderTargets, DFType rtFormat,
        unsigned int width, unsigned int height, bool hasRTMipmaps = false,
        bool createRTStorage = true, DFType dsFormat = DF_UNKNOWN,
        bool createDSStorage = false);

    // Member access.
    unsigned int GetNumTargets() const;
    DFType GetRTFormat() const;
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
    bool HasRTMipmaps() const;
    DFType GetDSFormat() const;
    eastl::shared_ptr<TextureRT> const GetRTTexture(unsigned int i) const;
	eastl::shared_ptr<TextureDS> const GetDSTexture() const;

    // Request that the GPU compute mipmap levels when the base-level render
    // target is written.  The AutogenerateMipmaps call should be made before
    // binding the texture to the engine.  If the texture does not have mipmaps,
    // the AutogenerateMipmaps call will not set mAutogenerateMipmaps to true.
    void AutogenerateRTMipmaps();
    bool IsAutogenerateRTMipmaps() const;

    // Listeners subscribe to receive notification when a DrawTarget is about
    // to be destroyed.  The intended use is for the DX11Engine objects to
    // destroy corresponding DX11DrawTarget objects.
    class GRAPHIC_ITEM ListenerForDestruction
    {
    public:
        virtual ~ListenerForDestruction() { /**/ }
        ListenerForDestruction() { /**/ }
        virtual void OnDestroy(DrawTarget const*) { /**/ }
    };

    static void SubscribeForDestruction(eastl::shared_ptr<ListenerForDestruction> const& listener);
    static void UnsubscribeForDestruction(eastl::shared_ptr<ListenerForDestruction> const& listener);

protected:
	eastl::vector<eastl::shared_ptr<TextureRT>> mRTTextures;
	eastl::shared_ptr<TextureDS> mDSTexture;

private:
    // Support for listeners for destruction (LFD).
    static std::recursive_mutex msLFDMutex;
    static eastl::set<eastl::shared_ptr<ListenerForDestruction>> msLFDSet;
};

#endif