// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef ALIGNEDBOX_H
#define ALIGNEDBOX_H

#include "Mathematic/Algebra/Vector.h"

// The box is aligned with the standard coordinate axes, which allows us to
// represent it using minimum and maximum values along each axis.  Some
// algorithms prefer the centered representation that is used for oriented
// boxes.  The center is C and the extents are the half-lengths in each
// coordinate-axis direction.

template <int N, typename Real>
class AlignedBox
{
public:
    // Construction and destruction.  The default constructor sets the
    // minimum values to -1 and the maximum values to +1.
    AlignedBox();

    // Please ensure that inMin[i] <= inMax[i] for all i.
    AlignedBox(Vector<N, Real> const& inMin, Vector<N, Real> const& inMax);

    // Compute the centered representation.  NOTE:  If you set the minimum
    // and maximum values, compute C and extents, and then recompute the
    // minimum and maximum values, the numerical round-off errors can lead to
    // results different from what you started with.
    void GetCenteredForm(Vector<N, Real>& center, Vector<N, Real>& extent) const;

    // Public member access.  It is required that min[i] <= max[i].
    Vector<N, Real> mMin, mMax;

public:
    // Comparisons to support sorted containers.
    bool operator==(AlignedBox const& box) const;
    bool operator!=(AlignedBox const& box) const;
    bool operator< (AlignedBox const& box) const;
    bool operator<=(AlignedBox const& box) const;
    bool operator> (AlignedBox const& box) const;
    bool operator>=(AlignedBox const& box) const;
};

// Template aliases for convenience.
template <typename Real>
using AlignedBox2 = AlignedBox<2, Real>;

template <typename Real>
using AlignedBox3 = AlignedBox<3, Real>;


template <int N, typename Real>
AlignedBox<N, Real>::AlignedBox()
{
    for (int i = 0; i < N; ++i)
    {
        mMin[i] = (Real)-1;
        mMax[i] = (Real)+1;
    }
}

template <int N, typename Real>
AlignedBox<N, Real>::AlignedBox(Vector<N, Real> const& inMin,
    Vector<N, Real> const& inMax)
{
    for (int i = 0; i < N; ++i)
    {
        mMin[i] = inMin[i];
        mMax[i] = inMax[i];
    }
}

template <int N, typename Real>
void AlignedBox<N, Real>::GetCenteredForm(Vector<N, Real>& center,
    Vector<N, Real>& extent) const
{
    center = (mMax + mMin) * (Real)0.5;
    extent = (mMax - mMin) * (Real)0.5;
}

template <int N, typename Real>
bool AlignedBox<N, Real>::operator==(AlignedBox const& box) const
{
    return mMin == box.mMin && mMax == box.mMax;
}

template <int N, typename Real>
bool AlignedBox<N, Real>::operator!=(AlignedBox const& box) const
{
    return !operator==(box);
}

template <int N, typename Real>
bool AlignedBox<N, Real>::operator<(AlignedBox const& box) const
{
    if (mMin < box.mMin)
    {
        return true;
    }

    if (mMin > box.mMin)
    {
        return false;
    }

    return mMax < box.mMax;
}

template <int N, typename Real>
bool AlignedBox<N, Real>::operator<=(AlignedBox const& box) const
{
    return operator<(box) || operator==(box);
}

template <int N, typename Real>
bool AlignedBox<N, Real>::operator>(AlignedBox const& box) const
{
    return !operator<=(box);
}

template <int N, typename Real>
bool AlignedBox<N, Real>::operator>=(AlignedBox const& box) const
{
    return !operator<(box);
}

#endif