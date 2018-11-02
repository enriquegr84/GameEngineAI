// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)


#include "Core/Logger/Logger.h"
#include "GL4ConstantBuffer.h"

GL4ConstantBuffer::GL4ConstantBuffer(ConstantBuffer const* cbuffer)
    :
    GL4Buffer(cbuffer, GL_UNIFORM_BUFFER)
{
    Initialize();
}

eastl::shared_ptr<GraphicObject> GL4ConstantBuffer::Create(void*, GraphicObject const* object)
{
    if (object->GetType() == GE_CONSTANT_BUFFER)
    {
        return eastl::make_shared<GL4ConstantBuffer>(
            static_cast<ConstantBuffer const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

void GL4ConstantBuffer::AttachToUnit(GLint uniformBufferUnit)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, uniformBufferUnit, mGLHandle);
}
