// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef TEXTURE2ARRAY_H
#define TEXTURE2ARRAY_H

#include "TextureArray.h"

class GRAPHIC_ITEM Texture2Array : public TextureArray
{
public:
    // Construction.
    Texture2Array(unsigned int numItems, DFType format, unsigned int width,
        unsigned int height, bool hasMipmaps = false,
        bool createStorage = true);

    // Texture dimensions.
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
};

#endif
