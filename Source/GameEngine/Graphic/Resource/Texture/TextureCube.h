// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef TEXTURECUBE_H
#define TEXTURECUBE_H

#include "TextureArray.h"

class GRAPHIC_ITEM TextureCube : public TextureArray
{
public:
    // Construction.  Cube maps must be square; the 'length' parameter is the
    // shared value for width and height.
    TextureCube(DFType format, unsigned int length, bool hasMipmaps = false, bool createStorage = true);

    // The texture width and height are the same value.
    unsigned int GetLength() const;
};

#endif
