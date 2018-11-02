// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)


#include "Core/Logger/Logger.h"
#include "GL4Texture3.h"


GL4Texture3::~GL4Texture3()
{
    glDeleteTextures(1, &mGLHandle);
}

GL4Texture3::GL4Texture3(Texture3 const* texture)
    :
    GL4TextureSingle(texture, GL_TEXTURE_3D, GL_TEXTURE_BINDING_3D)
{
    // Create a texture structure.
    glGenTextures(1, &mGLHandle);
    glBindTexture(GL_TEXTURE_3D, mGLHandle);

    // Allocate (immutable) texture storage for all levels.
    auto const width = texture->GetDimension(0);
    auto const height = texture->GetDimension(1);
    auto const depth = texture->GetDimension(2);
    glTexStorage3D(GL_TEXTURE_3D, mNumLevels, mInternalFormat, width, height, depth);

    Initialize();

    // Cannot leave this texture bound.
    glBindTexture(GL_TEXTURE_3D, 0);

    // Create a staging texture if requested.
    CreateStaging();
}

eastl::shared_ptr<GraphicObject> GL4Texture3::Create(void*, GraphicObject const* object)
{
    if (object->GetType() == GE_TEXTURE3)
    {
        return eastl::make_shared<GL4Texture3>(
            static_cast<Texture3 const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

bool GL4Texture3::CanAutoGenerateMipmaps() const
{
    auto texture = GetTexture();

    return texture && texture->HasMipmaps() && texture->IsAutogenerateMipmaps();
}

void GL4Texture3::LoadTextureLevel(unsigned int level, void const* data)
{
    auto texture = GetTexture();
    if (texture && level < texture->GetNumLevels())
    {
        auto width = texture->GetDimensionFor(level, 0);
        auto height = texture->GetDimensionFor(level, 1);
        auto depth = texture->GetDimensionFor(level, 2);

        glTexSubImage3D(GL_TEXTURE_3D, level, 0, 0, 0, width, height, depth,
            mExternalFormat, mExternalType, data);
    }
}
