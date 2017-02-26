// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Graphic/GraphicStd.h"

#include <EASTL/string.h>

class ControlledObject;

class GRAPHIC_ITEM Controller
{
protected:
    // Abstract base class.
    Controller();
public:
    virtual ~Controller();

    // Member access.
    inline ControlledObject* GetObject() const;
    inline void SetApplicationTime(double applicationTime);
    inline double GetApplicationTime() const;

    // The animation update.  The application time is in milliseconds.
    virtual bool Update(double applicationTime);

    // Time management.  A controller may use its own time scale, and it
    // specifies how times are to be mapped to application time.
    enum GRAPHIC_ITEM RepeatType
    {
        RT_CLAMP,
        RT_WRAP,
        RT_CYCLE
    };

    // Member access.
    RepeatType repeat;  // default = RT_CLAMP
    double minTime;     // default = 0
    double maxTime;     // default = 0
    double phase;       // default = 0
    double frequency;   // default = 1
    bool active;        // default = true

    // Allow user-readable names for nodes in a scene graph.
    eastl::string name;

public:  // INTERNAL USE ONLY
    // The class ControlledObject needs to set the object during a call to
    // AttachController.  Derived classes that manage a set of controllers
    // also need to set the objects during a call to AttachController.
    virtual void SetObject(ControlledObject* object);

protected:
    // Conversion from application time units to controller time units.
    // Derived classes may use this in their update routines.
    double GetControlTime(double applicationTime);

    // The controlled object.  This is a regular pointer rather than a
    // shared pointer to avoid the reference-count cycle between mObject
    // and 'this'.  Because the pointer links are set internally rather
    // than by an external manager, it is not possible to use std::weak_ptr
    // to avoid the cycle because we do not know the shared_ptr object
    // that owns mObject.
    ControlledObject* mObject;

    // The application time in milliseconds.
    double mApplicationTime;
};

#endif