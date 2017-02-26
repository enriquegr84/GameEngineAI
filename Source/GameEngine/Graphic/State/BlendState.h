// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef BLENDSTATE_H
#define BLENDSTATE_H

#include "Mathematic/Algebra/Vector4.h"
#include "DrawingState.h"

class GRAPHIC_ITEM BlendState : public DrawingState
{
public:
    enum GRAPHIC_ITEM Mode
    {
        BM_ZERO,
        BM_ONE,
        BM_SRC_COLOR,
        BM_INV_SRC_COLOR,
        BM_SRC_ALPHA,
        BM_INV_SRC_ALPHA,
        BM_DEST_ALPHA,
        BM_INV_DEST_ALPHA,
        BM_DEST_COLOR,
        BM_INV_DEST_COLOR,
        BM_SRC_ALPHA_SAT,
        BM_FACTOR,
        BM_INV_FACTOR,
        BM_SRC1_COLOR,
        BM_INV_SRC1_COLOR,
        BM_SRC1_ALPHA,
        BM_INV_SRC1_ALPHA
    };

    enum GRAPHIC_ITEM Operation
    {
        OP_ADD,
        OP_SUBTRACT,
        OP_REV_SUBTRACT,
        OP_MIN,
        OP_MAX
    };

    enum GRAPHIC_ITEM ColorWrite
    {
        CW_ENABLE_RED = 1,
        CW_ENABLE_GREEN = 2,
        CW_ENABLE_BLUE = 4,
        CW_ENABLE_ALPHA = 8,
        CW_ENABLE_ALL = 15
    };

    enum GRAPHIC_ITEM
    {
        NUM_TARGETS = 8
    };

    struct GRAPHIC_ITEM Target
    {
        bool enable;        // default: false
        Mode srcColor;      // default: BM_ONE
        Mode dstColor;      // default: BM_ZERO
        Operation opColor;  // default: OP_ADD
        Mode srcAlpha;      // default: BM_ONE
        Mode dstAlpha;      // default: BM_ZERO
        Operation opAlpha;  // default: OP_ADD
        unsigned char mask; // default: CW_ENABLE_ALL
    };

    // Construction.
    BlendState();

    // Member access.  The members are intended to be write-once before
    // you create an associated graphics state.
    bool enableAlphaToCoverage;     // default: false
    bool enableIndependentBlend;    // default: false
    Target target[NUM_TARGETS];
    Vector4<float> blendColor;      // default: (0,0,0,0)
    unsigned int sampleMask;        // default: 0xFFFFFFFF
};

#endif