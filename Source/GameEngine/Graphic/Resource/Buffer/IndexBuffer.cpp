// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/12/09)

#include "Core/Logger/Logger.h"
#include "Mathematic/Arithmetic/BitHacks.h"

#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(IPType type, uint32_t numPrimitives, size_t indexSize, bool createStorage)
    :
    Buffer(msIndexCounter[Log2OfPowerOfTwo(type)](numPrimitives), indexSize, createStorage),
    mPrimitiveType(type),
    mNumPrimitives(numPrimitives),
    mNumActivePrimitives(numPrimitives),
    mFirstPrimitive(0)
{
    mType = GE_INDEX_BUFFER;
    LogAssert(mNumPrimitives > 0, "Invalid number of primitives.");
}

IndexBuffer::IndexBuffer(IPType type, uint32_t numPrimitives)
    :
    Buffer(msIndexCounter[Log2OfPowerOfTwo(type)](numPrimitives), 0, false),
    mPrimitiveType(type),
    mNumPrimitives(numPrimitives),
    mNumActivePrimitives(numPrimitives),
    mFirstPrimitive(0)
{
    mType = GE_INDEX_BUFFER;
    LogAssert(mNumPrimitives > 0, "Invalid number of primitives.");
}

void IndexBuffer::SetNumActivePrimitives(uint32_t numActive)
{
    if (numActive <= mNumPrimitives)
    {
        mNumActivePrimitives = numActive;
    }
    else
    {
        LogWarning("Invalid number of active primitives.");
        mNumActivePrimitives = mNumPrimitives;
    }
}

uint32_t IndexBuffer::GetNumActiveIndices() const
{
    uint32_t i = Log2OfPowerOfTwo(mPrimitiveType);
    return msIndexCounter[i](mNumActivePrimitives);
}

void IndexBuffer::SetFirstPrimitive(uint32_t first)
{
    if (0 <= first && first < mNumPrimitives && first + mNumActivePrimitives <= mNumPrimitives)
    {
        mFirstPrimitive = first;
        return;
    }

    LogError("Invalid first primitive.");
}

uint32_t IndexBuffer::GetFirstIndex() const
{
    if (mFirstPrimitive == 0)
    {
        return 0;
    }

    int i = Log2OfPowerOfTwo(mPrimitiveType);
    return msIndexCounter[i](mFirstPrimitive);
}

bool IndexBuffer::SetPoint(uint32_t i, uint32_t v)
{
    if (ValidPrimitiveType(IP_HAS_POINTS))
    {
        if (mData && i < mNumPrimitives)
        {
            if (mElementSize == sizeof(uint32_t))
            {
                uint32_t* index = i + Get<uint32_t>();
                *index = v;
            }
            else
            {
                uint16_t* index = i + Get<uint16_t>();
                *index = static_cast<uint16_t>(v);
            }
            return true;
        }
    }
    return false;
}

bool IndexBuffer::GetPoint(uint32_t i, uint32_t& v) const
{
    if (ValidPrimitiveType(IP_HAS_POINTS))
    {
        if (mData && i < mNumPrimitives)
        {
            if (mElementSize == sizeof(uint32_t))
            {
                uint32_t const* index = i + Get<uint32_t>();
                v = *index;
            }
            else
            {
                uint16_t const* index = i + Get<uint16_t>();
                v = static_cast<uint32_t>(*index);
            }
            return true;
        }
    }
    return false;
}

bool IndexBuffer::SetSegment(uint32_t i, uint32_t v0, uint32_t v1)
{
    if (ValidPrimitiveType(IP_HAS_SEGMENTS))
    {
        if (mData && i < mNumPrimitives)
        {
            if (mElementSize == sizeof(uint32_t))
            {
                if (mPrimitiveType == IP_POLYSEGMENT_DISJOINT)
                {
                    uint32_t* index = 2 * i + Get<uint32_t>();
                    *index++ = v0;
                    *index = v1;
                }
                else
                {
                    uint32_t* index = i + Get<uint32_t>();
                    *index++ = v0;
                    *index = v1;
                }
            }
            else
            {
                if (mPrimitiveType == IP_POLYSEGMENT_DISJOINT)
                {
                    uint16_t* index = 2 * i + Get<uint16_t>();
                    *index++ = static_cast<uint16_t>(v0);
                    *index = static_cast<uint16_t>(v1);
                }
                else
                {
                    uint16_t* index = i + Get<uint16_t>();
                    *index++ = static_cast<uint16_t>(v0);
                    *index = static_cast<uint16_t>(v1);
                }
            }
            return true;
        }
    }
    return false;
}

bool IndexBuffer::GetSegment(uint32_t i, uint32_t& v0, uint32_t& v1) const
{
    if (ValidPrimitiveType(IP_HAS_SEGMENTS))
    {
        if (mData && i < mNumPrimitives)
        {
            if (mElementSize == sizeof(uint32_t))
            {
                if (mPrimitiveType == IP_POLYSEGMENT_DISJOINT)
                {
                    uint32_t const* index = 2 * i + Get<uint32_t>();
                    v0 = *index++;
                    v1 = *index;
                }
                else
                {
                    uint32_t const* index = i + Get<uint32_t>();
                    v0 = *index++;
                    v1 = *index;
                }
            }
            else
            {
                if (mPrimitiveType == IP_POLYSEGMENT_DISJOINT)
                {
                    uint16_t const* index = 2 * i + Get<uint16_t>();
                    v0 = static_cast<uint32_t>(*index++);
                    v1 = static_cast<uint32_t>(*index);
                }
                else
                {
                    uint16_t const* index = i + Get<uint16_t>();
                    v0 = static_cast<uint32_t>(*index++);
                    v1 = static_cast<uint32_t>(*index);
                }
            }
            return true;
        }
    }
    return false;
}

bool IndexBuffer::SetTriangle(uint32_t i, uint32_t v0, uint32_t v1, uint32_t v2)
{
    if (ValidPrimitiveType(IP_HAS_TRIANGLES))
    {
        if (mData && i < mNumPrimitives)
        {
            if (mElementSize == sizeof(uint32_t))
            {
                if (mPrimitiveType == IP_TRIMESH)
                {
                    uint32_t* index = 3 * i + Get<uint32_t>();
                    *index++ = v0;
                    *index++ = v1;
                    *index = v2;
                }
                else
                {
                    uint32_t* index = i + Get<uint32_t>();
                    index[0] = v0;
                    if (i & 1)
                    {
                        index[2] = v1;
                        index[1] = v2;
                    }
                    else
                    {
                        index[1] = v1;
                        index[2] = v2;
                    }
                }
            }
            else
            {
                if (mPrimitiveType == IP_TRIMESH)
                {
                    uint16_t* index = 3 * i + Get<uint16_t>();
                    *index++ = static_cast<uint16_t>(v0);
                    *index++ = static_cast<uint16_t>(v1);
                    *index = static_cast<uint16_t>(v2);
                }
                else
                {
                    uint16_t* index = i + Get<uint16_t>();
                    index[0] = static_cast<uint16_t>(v0);
                    if (i & 1)
                    {
                        index[2] = static_cast<uint16_t>(v1);
                        index[1] = static_cast<uint16_t>(v2);
                    }
                    else
                    {
                        index[1] = static_cast<uint16_t>(v1);
                        index[2] = static_cast<uint16_t>(v2);
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool IndexBuffer::GetTriangle(uint32_t i, uint32_t& v0, uint32_t& v1, uint32_t& v2) const
{
    if (ValidPrimitiveType(IP_HAS_TRIANGLES))
    {
        if (mData && i < mNumPrimitives)
        {
            if (mElementSize == sizeof(uint32_t))
            {
                if (mPrimitiveType == IP_TRIMESH)
                {
                    uint32_t const* index = 3 * i + Get<uint32_t>();
                    v0 = *index++;
                    v1 = *index++;
                    v2 = *index;
                }
                else
                {
                    uint32_t const* index = i + Get<uint32_t>();
                    uint32_t offset = (i & 1);
                    v0 = index[0];
                    v1 = index[1 + offset];
                    v2 = index[2 - offset];
                }
            }
            else
            {
                if (mPrimitiveType == IP_TRIMESH)
                {
                    uint16_t const* index =
                        3 * i + Get<uint16_t>();
                    v0 = static_cast<uint32_t>(*index++);
                    v1 = static_cast<uint32_t>(*index++);
                    v2 = static_cast<uint32_t>(*index);
                }
                else
                {
                    uint16_t const* index = i + Get<uint16_t>();
                    int offset = (i & 1);
                    v0 = static_cast<uint32_t>(index[0]);
                    v1 = static_cast<uint32_t>(index[1 + offset]);
                    v2 = static_cast<uint32_t>(index[2 - offset]);
                }
            }
            return true;
        }
    }
    return false;
}

uint32_t IndexBuffer::GetPolypointIndexCount(uint32_t numPrimitives)
{
    // Create one point when numPrimitives is invalid.
    return numPrimitives > 0 ? numPrimitives : 1;
}

uint32_t IndexBuffer::GetPolysegmentDisjointIndexCount(
    uint32_t numPrimitives)
{
    // Create one segment when numPrimitives is invalid.
    return numPrimitives > 0 ? 2 * numPrimitives : 2;
}

uint32_t IndexBuffer::GetPolysegmentContiguousIndexCount(
    uint32_t numPrimitives)
{
    // Create one segment when numPrimitives is invalid.
    return numPrimitives > 0 ? numPrimitives + 1 : 2;
}

uint32_t IndexBuffer::GetTrimeshIndexCount(uint32_t numPrimitives)
{
    // Create one triangle when numPrimitives is invalid.
    return numPrimitives > 0 ? 3 * numPrimitives : 3;
}

uint32_t IndexBuffer::GetTristripIndexCount(uint32_t numPrimitives)
{
    // Create one triangle when numPrimitives is invalid.
    return numPrimitives > 0 ? numPrimitives + 2 : 3;
}

uint32_t IndexBuffer::GetPolysegmentDisjointAdjIndexCount(uint32_t numPrimitives)
{
    // Create one segment-adj when numPrimitives is invalid.
    return numPrimitives > 0 ? 4 * numPrimitives : 4;
}

uint32_t IndexBuffer::GetPolysegmentContiguousAdjIndexCount(uint32_t numPrimitives)
{
    // Create one segment-adj when numPrimitives is invalid.
    return numPrimitives > 0 ? numPrimitives + 3 : 4;
}

uint32_t IndexBuffer::GetTrimeshAdjIndexCount(uint32_t numPrimitives)
{
    // Create one triangle-adj when numPrimitives is invalid.
    return numPrimitives > 0 ? 6 * numPrimitives : 6;
}

uint32_t IndexBuffer::GetTristripAdjIndexCount(uint32_t numPrimitives)
{
    // Create one triangle-adj when numPrimitives is invalid.
    return numPrimitives > 0 ? 2 * (numPrimitives + 2) : 6;
}

IndexBuffer::ICFunction IndexBuffer::msIndexCounter[IP_NUM_TYPES] =
{
    &IndexBuffer::GetPolypointIndexCount,
    &IndexBuffer::GetPolysegmentDisjointIndexCount,
    &IndexBuffer::GetPolysegmentContiguousIndexCount,
    &IndexBuffer::GetTrimeshIndexCount,
    &IndexBuffer::GetTristripIndexCount,
    &IndexBuffer::GetPolysegmentDisjointAdjIndexCount,
    &IndexBuffer::GetPolysegmentContiguousAdjIndexCount,
    &IndexBuffer::GetTrimeshAdjIndexCount,
    &IndexBuffer::GetTristripAdjIndexCount
};
