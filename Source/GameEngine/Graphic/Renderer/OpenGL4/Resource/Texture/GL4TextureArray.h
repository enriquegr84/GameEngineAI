// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef GL4TEXTUREARRAY_H
#define GL4TEXTUREARRAY_H

#include "Graphic/Resource/Texture/TextureArray.h"
#include "GL4Texture.h"

#include "Core/Logger/Logger.h"

class GRAPHIC_ITEM GL4TextureArray : public GL4Texture
{
public:
    // Abstract base class, a shim to distinguish between single textures and
    // texture arrays.
    virtual ~GL4TextureArray();

    // Member access.
    inline TextureArray* GetTexture() const;

    virtual bool Update() override;
    virtual bool CopyCpuToGpu() override;
    virtual bool CopyGpuToCpu() override;

    bool Update(unsigned int item, unsigned int level);
    bool CopyCpuToGpu(unsigned int item, unsigned int level);
    bool CopyGpuToCpu(unsigned int item, unsigned int level);
    void CopyGpuToGpu(GL4TextureArray* target, unsigned int item, unsigned int level)
    {
        (void)target;
        (void)item;
        (void)level;
        LogError("Not yet implemented.");
    }

    // Returns true if mipmaps need to be generated.
    virtual bool CanAutoGenerateMipmaps() const = 0;

    // Generates mipmaps from level 0 -- only if CanAutoGenerateMipmaps() returns true.
    virtual bool GenerateMipmaps();

protected:
    // No public construction.  Derived classes use this constructor.
    GL4TextureArray(TextureArray const* gtTexture, GLenum target, GLenum targetBinding);

    // Only call from derived class constructor after texture storage has been allocated.
    void Initialize();

    // Called by Update and CopyCpuToGpu.
    bool DoCopyCpuToGpu(unsigned int item, unsigned int level);

    // Should be called in constructor when CopyType is any value bu COPY_NONE.
    void CreateStaging();

    // This is called to copy the data from the CPU buffer to the GPU
    // for the specified level.  If a pixel unpack buffer is being used
    // then data needs to be passed as 0 which is used as an offset.
    virtual void LoadTextureLevel(unsigned int item, unsigned int level, void const* data) = 0;

    // Conversions from Engine values to GL4 values.
    static GLenum const msCubeFaceTarget[6];

private:
    // Data associated with each mip level
    GLuint mLevelPixelUnpackBuffer[Texture::MAX_MIPMAP_LEVELS];
    GLuint mLevelPixelPackBuffer[Texture::MAX_MIPMAP_LEVELS];
};

inline TextureArray* GL4TextureArray::GetTexture() const
{
    return static_cast<TextureArray*>(mGObject);
}

#endif