// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef GL4RESOURCE_H
#define GL4RESOURCE_H

#include "Graphic/Resource/Resource.h"
#include "Graphic/Renderer/OpenGL4/Resource/GL4GraphicObject.h"
#include "Core/Logger/Logger.h"

class GRAPHIC_ITEM GL4Resource : public GL4GraphicObject
{
protected:
    // Abstract base class.
    GL4Resource(Resource const* gtResource);

public:
    // Member access.
    inline Resource* GetResource() const;

    // TODO: TENTATIVE INTERFACE (modify as needed).  Make these pure
    // virtual latter (if relevant).
    void* MapForWrite(GLenum target);
    void Unmap(GLenum target);
    virtual bool Update() { return false;  }
    virtual bool CopyCpuToGpu() { return false; }
    virtual bool CopyGpuToCpu() { return false; }

    virtual void CopyGpuToGpu(GL4Resource* target)
    {
        (void)target;
        LogError("Not yet implemented.");
    }

protected:
    // Support for copying between CPU and GPU.
    bool PreparedForCopy(GLenum access) const;
};

inline Resource* GL4Resource::GetResource() const
{
    return static_cast<Resource*>(mGObject);
}

#endif