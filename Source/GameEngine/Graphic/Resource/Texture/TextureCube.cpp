// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "TextureCube.h"

TextureCube::TextureCube(DFType format, unsigned int length, bool hasMipmaps, bool createStorage)
    :
    TextureArray(CubeFaceCount, format, 2, length, length, 1, hasMipmaps, createStorage)
{
    mType = GE_TEXTURE_CUBE;
}

unsigned int TextureCube::GetLength() const
{
    return TextureArray::GetDimension(0);
}

