// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef TEXTURERT_H
#define TEXTURERT_H

#include "Texture2.h"

class GRAPHIC_ITEM TextureRT : public Texture2
{
public:
    // Construction for render targets.
    TextureRT(DFType format, unsigned int width, unsigned int height,
        bool hasMipmaps = false, bool createStorage = true);
};

#endif