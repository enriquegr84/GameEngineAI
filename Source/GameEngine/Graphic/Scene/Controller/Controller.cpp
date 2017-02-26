// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "Controller.h"

#include <cmath>
#include <limits>

Controller::Controller()
    :
    repeat(RT_CLAMP),
    minTime(0.0),
    maxTime(0.0),
    phase(0.0),
    frequency(1.0),
    active(true),
    name(""),
    mObject(nullptr),
    mApplicationTime(-std::numeric_limits<double>::max())
{
}

Controller::~Controller()
{
}

bool Controller::Update(double applicationTime)
{
    if (active)
    {
        mApplicationTime = applicationTime;
        return true;
    }
    return false;
}

void Controller::SetObject(ControlledObject* object)
{
    mObject = object;
}

double Controller::GetControlTime(double applicationTime)
{
    double controlTime = frequency * applicationTime + phase;

    if (repeat == RT_CLAMP)
    {
        // Clamp the time to the [min,max] interval.
        if (controlTime < minTime)
        {
            return minTime;
        }
        if (controlTime > maxTime)
        {
            return maxTime;
        }
        return controlTime;
    }

    double timeRange = maxTime - minTime;
    if (timeRange > 0.0)
    {
        double multiples = (controlTime - minTime) / timeRange;
        double integerTime = floor(multiples);
        double fractionTime = multiples - integerTime;
        if (repeat == RT_WRAP)
        {
            return minTime + fractionTime*timeRange;
        }

        // repeat == RT_CYCLE
        if (static_cast<int>(integerTime) & 1)
        {
            // Go backward in time.
            return maxTime - fractionTime * timeRange;
        }
        else
        {
            // Go forward in time.
            return minTime + fractionTime * timeRange;
        }
    }

    // The minimum and maximum times are the same, so return the minimum.
    return minTime;
}
