// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)


#include "Core/Logger/Logger.h"
#include "GL4Texture1.h"


GL4Texture1::~GL4Texture1()
{
    glDeleteTextures(1, &mGLHandle);
}

GL4Texture1::GL4Texture1(Texture1 const* texture)
    :
    GL4TextureSingle(texture, GL_TEXTURE_1D, GL_TEXTURE_BINDING_1D)
{
    // Create a texture structure.
    glGenTextures(1, &mGLHandle);
    glBindTexture(GL_TEXTURE_1D, mGLHandle);

    // Allocate (immutable) texture storage for all levels.
    auto const length = texture->GetDimension(0);
    glTexStorage1D(GL_TEXTURE_1D, mNumLevels, mInternalFormat, length);

    Initialize();

    // Cannot leave this texture bound.
    glBindTexture(GL_TEXTURE_1D, 0);

    // Create a staging texture if requested.
    CreateStaging();
}

eastl::shared_ptr<GraphicObject> GL4Texture1::Create(void*, GraphicObject const* object)
{
    if (object->GetType() == GE_TEXTURE1)
    {
        return eastl::make_shared<GL4Texture1>(
            static_cast<Texture1 const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

bool GL4Texture1::CanAutoGenerateMipmaps() const
{
    auto texture = GetTexture();

    return texture && texture->HasMipmaps() && texture->IsAutogenerateMipmaps();
}

void GL4Texture1::LoadTextureLevel(unsigned int level, void const* data)
{
    auto texture = GetTexture();
    if (texture && level < texture->GetNumLevels())
    {
        auto length = texture->GetDimensionFor(level, 0);

        glTexSubImage1D(GL_TEXTURE_1D, level, 0, length,
            mExternalFormat, mExternalType, data);
    }
}
