// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef HLSLCOMPUTEPROGRAM_H
#define HLSLCOMPUTEPROGRAM_H

#include "Graphic/Shader/ComputeProgram.h"

class GRAPHIC_ITEM HLSLComputeProgram : public ComputeProgram
{
public:
    // A simple stub to add HLSL as part of the program type.  This allows
    // polymorphism for the program factory classes, which in turn allows
    // us to hide the graphics-API-dependent program factory used by the
    // Window class (have a member mProgramFactory similar to mEngine that
    // is created according to the desired graphics API).
    virtual ~HLSLComputeProgram();
    HLSLComputeProgram();
};

#endif