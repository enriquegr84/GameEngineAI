// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/11/13)

#ifndef TEXTUREDS_H
#define TEXTUREDS_H

#include "Texture2.h"

class GRAPHIC_ITEM TextureDS : public Texture2
{
public:
    // Construction for depth-stencil textures.
    TextureDS(DFType format, unsigned int width, unsigned int height,
        bool createStorage = true);

    void MakeShaderInput();
    bool IsShaderInput() const;

private:
    bool mShaderInput;
};

#endif