// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "BlendState.h"

BlendState::BlendState()
    :
    mEnableAlphaToCoverage(false),
    mEnableIndependentBlend(false),
    mBlendColor({ 0.0f, 0.0f, 0.0f, 0.0f }),
    mSampleMask(0xFFFFFFFFu)
{
    mType = GE_BLEND_STATE;

    for (int i = 0; i < NUM_TARGETS; ++i)
    {
        Target& trg = mTarget[i];
        trg.enable = false;
        trg.srcColor = BM_ONE;
        trg.dstColor = BM_ZERO;
        trg.opColor = OP_ADD;
        trg.srcAlpha = BM_ONE;
        trg.dstAlpha = BM_ZERO;
        trg.opAlpha = OP_ADD;
        trg.mask = CW_ENABLE_ALL;
    }
}

