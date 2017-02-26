// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/09/12)

#ifndef HLSLPARAMETER_H
#define HLSLPARAMETER_H

#include "HLSLBaseBuffer.h"
#include <fstream>

class GRAPHIC_ITEM HLSLParameter
{
public:
    struct Description
    {
		eastl::string semanticName;
        unsigned int semanticIndex;
        unsigned int registerIndex;
        D3D_NAME systemValueType;
        D3D_REGISTER_COMPONENT_TYPE componentType;
        unsigned int mask;
        unsigned int readWriteMask;
        unsigned int stream;
        D3D_MIN_PRECISION minPrecision;
    };

    // Construction.  Parameters are reported for inputs, outputs, and patch
    // constants.
    HLSLParameter(D3D_SIGNATURE_PARAMETER_DESC const& desc);

    // Member access.
	eastl::string const& GetSemanticName() const;
    unsigned int GetSemanticIndex() const;
    unsigned int GetRegisterIndex() const;
    D3D_NAME GetSystemValueType() const;
    D3D_REGISTER_COMPONENT_TYPE GetComponentType() const;
    unsigned int GetMask() const;
    unsigned int GetReadWriteMask() const;
    unsigned int GetStream() const;
    D3D_MIN_PRECISION GetMinPrecision() const;

    // Print to a text file for human readability.
    void Print(std::ofstream& output) const;

private:
    Description mDesc;

    // Support for Print.
    static eastl::string const msSVName[];
    static eastl::string const msComponentType[];
    static eastl::string const msMinPrecision[];
};

#endif