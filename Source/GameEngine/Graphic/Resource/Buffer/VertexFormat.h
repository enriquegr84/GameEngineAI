// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef VERTEXFORMAT_H
#define VERTEXFORMAT_H

#include "Graphic/Resource/DataFormat.h"

// Enumerations for DX11.
enum GRAPHIC_ITEM VASemantic
{
    VA_NO_SEMANTIC,
    VA_POSITION,
    VA_BLENDWEIGHT,
    VA_BLENDINDICES,
    VA_NORMAL,
    VA_PSIZE,
    VA_TEXCOORD,
    VA_TANGENT,
    VA_BINORMAL,
    VA_TESSFACTOR,
    VA_POSITIONT,
    VA_COLOR,
    VA_FOG,
    VA_DEPTH,
    VA_SAMPLE,
    VA_NUM_SEMANTICS
};

enum GRAPHIC_ITEM VAConstant
{
    // TODO:  Modify to the numbers for Shader Model 5 (DX11).

    // The maximum number of attributes for a vertex format.
    VA_MAX_ATTRIBUTES = 16,

    // The maximum number of texture coordinate units.
    VA_MAX_TCOORD_UNITS = 8,

    // The maximum number of color units.
    VA_MAX_COLOR_UNITS = 2
};

enum GRAPHIC_ITEM VertexType
{
	//! Standard vertex type used by the engine
	VT_STANDARD = 0,

	//! Vertex with two texture coordinates
	/** Usually used for geometry with lightmaps or other special materials. */
	VT_2TCOORDS,

	//! Vertex with a tangent and binormal vector
	/** Usually used for tangent space normal mapping. */
	VT_TANGENTS
};

class GRAPHIC_ITEM VertexFormat
{
public:
    // Construction.
    VertexFormat();

    // Create a packed vertex format, where all attributes are contiguous in
    // memory.  The order of the attributes is determined by the order of
    // Bind calls.
    bool Bind(VASemantic semantic, DFType type, unsigned int unit);

    // Member access.  GetAttribute returns 'true' when the input i is
    // such that 0 <= i < GetNumAttributes(), in which case the returned
    // semantic, type, unit, and offset are valid.
    unsigned int GetVertexSize() const;
    int GetNumAttributes() const;
    bool GetAttribute(int i, VASemantic& semantic, DFType& type,
        unsigned int& unit, unsigned int& offset) const;

    // Determine whether a semantic/unit exists.  If so, return the
    // index i that can be used to obtain more information about the
    // attribute by the functions after this.  If not, return -1.
    int GetIndex(VASemantic semantic, unsigned int unit) const;
    DFType GetType(int i) const;
    unsigned int GetOffset(int i) const;

private:
    class GRAPHIC_ITEM Attribute
    {
    public:
        Attribute();
        VASemantic semantic;
        DFType type;
        unsigned int unit;
        unsigned int offset;
    };

    int mNumAttributes;
    unsigned int mVertexSize;
    Attribute mAttributes[VA_MAX_ATTRIBUTES];
};

#endif
