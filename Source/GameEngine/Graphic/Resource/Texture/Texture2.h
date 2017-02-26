// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef TEXTURE2_H
#define TEXTURE2_H

#include "TextureSingle.h"

class GRAPHIC_ITEM Texture2 : public TextureSingle
{
public:
    // Construction.
    Texture2(DFType format, unsigned int width, unsigned int height,
        bool hasMipmaps = false, bool createStorage = true);

    // Texture dimensions.
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;

    // If you intend to share this texture among DX11Engine objects, call this
    // function before binding the texture to the engine.  The Texture2 usage
    // must be Resource::GPU_RW.
    void MakeShared();
    bool IsShared() const;

protected:
    bool mShared;
};

#endif
