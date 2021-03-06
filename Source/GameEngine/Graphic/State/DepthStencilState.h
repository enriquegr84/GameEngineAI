// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/10/02)

#ifndef DEPTHSTENCILSTATE_H
#define DEPTHSTENCILSTATE_H

#include "DrawingState.h"

class GRAPHIC_ITEM DepthStencilState : public DrawingState
{
public:
    enum GRAPHIC_ITEM WriteMask
    {
        MASK_ZERO,
        MASK_ALL
    };

    enum GRAPHIC_ITEM Comparison
    {
        NEVER,
        LESS,
        EQUAL,
        LESS_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_EQUAL,
        ALWAYS
    };

    enum GRAPHIC_ITEM Operation
    {
        OP_KEEP,
        OP_ZERO,
        OP_REPLACE,
        OP_INCR_SAT,
        OP_DECR_SAT,
        OP_INVERT,
        OP_INCR,
        OP_DECR
    };

    struct GRAPHIC_ITEM Face
    {
        Operation fail;
        Operation depthFail;
        Operation pass;
        Comparison comparison;
    };

    // Construction.
    DepthStencilState();

    // Member access.  The members are intended to be write-once before
    // you create an associated graphics state.
    bool mDepthEnable;                   // default: true
    WriteMask mWriteMask;                // default: MASK_ALL
    Comparison mComparison;              // default: LESS_EQUAL
    bool mStencilEnable;                 // default: false
    unsigned char mStencilReadMask;      // default: 0xFF
    unsigned char mStencilWriteMask;     // default: 0xFF
    Face mFrontFace;                     // default: (KEEP,KEEP,KEEP,ALWAYS)
    Face mBackFace;                      // default: (KEEP,KEEP,KEEP,ALWAYS)
    unsigned int mReference;             // default: 0
};

#endif