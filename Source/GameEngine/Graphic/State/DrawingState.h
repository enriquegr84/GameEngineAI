// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef DRAWINGSTATE_H
#define DRAWINGSTATE_H

#include "Graphic/Resource/GraphicObject.h"


class GRAPHIC_ITEM DrawingState : public GraphicObject
{
protected:
    // Abstract base class for grouping state classes.  This supports
    // simplification and reduction of member functions in the graphics engine
    // code.
    DrawingState();
};

#endif
