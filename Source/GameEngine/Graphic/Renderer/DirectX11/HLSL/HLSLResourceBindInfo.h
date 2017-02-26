// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/09/12)

#ifndef HLSLRESOURCEBINDINFO_H
#define HLSLRESOURCEBINDINFO_H

#include "HLSLBaseBuffer.h"

class GRAPHIC_ITEM HLSLResourceBindInfo : public HLSLBaseBuffer
{
public:
    // Construction and destruction.
    virtual ~HLSLResourceBindInfo();

    HLSLResourceBindInfo(D3D_SHADER_INPUT_BIND_DESC const& desc,
        unsigned int numBytes, eastl::vector<Member> const& members);

    HLSLResourceBindInfo(D3D_SHADER_INPUT_BIND_DESC const& desc,
        unsigned int index, unsigned int numBytes,
		eastl::vector<Member> const& members);
};

#endif