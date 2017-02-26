// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/11/13)

#include "TextureDS.h"

TextureDS::TextureDS(DFType format, unsigned int width, unsigned int height, 
	bool createStorage)
    :
    Texture2(DataFormat::IsDepth(format) ? format : DF_D24_UNORM_S8_UINT,
        width, height, false, createStorage),
    mShaderInput(false)
{
    mType = GE_TEXTURE_DS;
}

void TextureDS::MakeShaderInput()
{
    mShaderInput = true;
}

bool TextureDS::IsShaderInput() const
{
    return mShaderInput;
}
