// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)


#include "Core/Logger/Logger.h"
#include "GL4AtomicCounterBuffer.h"

GL4AtomicCounterBuffer::GL4AtomicCounterBuffer(RawBuffer const* cbuffer)
    :
    GL4Buffer(cbuffer, GL_ATOMIC_COUNTER_BUFFER)
{
    Initialize();
}

eastl::shared_ptr<GraphicObject> GL4AtomicCounterBuffer::Create(void*, GraphicObject const* object)
{
    if (object->GetType() == GE_RAW_BUFFER)
    {
        return eastl::make_shared<GL4AtomicCounterBuffer>(
            static_cast<RawBuffer const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}

void GL4AtomicCounterBuffer::AttachToUnit(GLint atomicCounterBufferUnit)
{
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBufferUnit, mGLHandle);
}
