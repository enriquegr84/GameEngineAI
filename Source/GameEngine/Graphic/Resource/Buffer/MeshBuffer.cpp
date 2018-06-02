// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "MeshBuffer.h"

MeshBuffer::MeshBuffer(unsigned int numVertices, unsigned int numIndices,
	uint32_t numPrimitives, size_t meshSize, bool createStorage)
	: Buffer(numPrimitives, meshSize, createStorage)
{
	MeshDescription desc(numVertices, numIndices);
	mMesh = eastl::make_shared<Mesh<float>>(desc, eastl::vector<MeshTopology>{ desc.mTopology });
}

MeshBuffer::~MeshBuffer()
{

}