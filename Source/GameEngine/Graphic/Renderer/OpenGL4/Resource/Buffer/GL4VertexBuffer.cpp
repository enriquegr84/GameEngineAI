// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)


#include "Core/Logger/Logger.h"
#include "GL4VertexBuffer.h"


GL4VertexBuffer::GL4VertexBuffer(VertexBuffer const* vbuffer)
    :
    GL4Buffer(vbuffer, GL_ARRAY_BUFFER)
{
    Initialize();
}

eastl::shared_ptr<GraphicObject> GL4VertexBuffer::Create(void*, GraphicObject const* object)
{
    if (object->GetType() == GE_VERTEX_BUFFER)
    {
        return eastl::make_shared<GL4VertexBuffer>(
            static_cast<VertexBuffer const*>(object));
    }

    LogError("Invalid object type.");
    return nullptr;
}
