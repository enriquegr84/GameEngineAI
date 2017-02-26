// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/09/12)

#include "HLSLParameter.h"

HLSLParameter::HLSLParameter(D3D_SIGNATURE_PARAMETER_DESC const& desc)
{
    mDesc.semanticName = eastl::string(desc.SemanticName);
    mDesc.semanticIndex = desc.SemanticIndex;
    mDesc.registerIndex = desc.Register;
    mDesc.systemValueType = desc.SystemValueType;
    mDesc.componentType = desc.ComponentType;
    mDesc.mask = desc.Mask;
    mDesc.readWriteMask = desc.ReadWriteMask;
    mDesc.stream = desc.Stream;
    mDesc.minPrecision = desc.MinPrecision;
}

eastl::string const& HLSLParameter::GetSemanticName() const
{
    return mDesc.semanticName;
}

unsigned int HLSLParameter::GetSemanticIndex() const
{
    return mDesc.semanticIndex;
}

unsigned int HLSLParameter::GetRegisterIndex() const
{
    return mDesc.registerIndex;
}

D3D_NAME HLSLParameter::GetSystemValueType() const
{
    return mDesc.systemValueType;
}

D3D_REGISTER_COMPONENT_TYPE HLSLParameter::GetComponentType() const
{
    return mDesc.componentType;
}

unsigned int HLSLParameter::GetMask() const
{
    return mDesc.mask;
}

unsigned int HLSLParameter::GetReadWriteMask() const
{
    return mDesc.readWriteMask;
}

unsigned int HLSLParameter::GetStream() const
{
    return mDesc.stream;
}

D3D_MIN_PRECISION HLSLParameter::GetMinPrecision() const
{
    return mDesc.minPrecision;
}

void HLSLParameter::Print(std::ofstream& output) const
{
    output << "semantic name = " << mDesc.semanticName.c_str() << std::endl;
    output << "semantic index = " << mDesc.semanticIndex << std::endl;
    output << "register index = " << mDesc.registerIndex << std::endl;
    output << "system value type = "
        << msSVName[mDesc.systemValueType].c_str() << std::endl;
    output << "register component type = "
        << msComponentType[mDesc.componentType].c_str() << std::endl;

    output << std::hex << std::showbase;
    output << "mask = " << mDesc.mask << std::endl;
    output << "read-write mask = " << mDesc.readWriteMask << std::endl;
    output << std::dec << std::noshowbase;

    output << "stream = " << mDesc.stream << std::endl;

    int i = static_cast<int>(mDesc.minPrecision);
    if (i & 0x000000F0)
    {
        i = 6 + (i & 1);
    }
    output << "min precision = " << msMinPrecision[i].c_str() << std::endl;
}


eastl::string const HLSLParameter::msSVName[] =
{
    "D3D_NAME_UNDEFINED",
    "D3D_NAME_POSITION",
    "D3D_NAME_CLIP_DISTANCE",
    "D3D_NAME_CULL_DISTANCE",
    "D3D_NAME_RENDER_TARGET_ARRAY_INDEX",
    "D3D_NAME_VIEWPORT_ARRAY_INDEX",
    "D3D_NAME_VERTEX_ID",
    "D3D_NAME_PRIMITIVE_ID",
    "D3D_NAME_INSTANCE_ID",
    "D3D_NAME_IS_FRONT_FACE",
    "D3D_NAME_SAMPLE_INDEX",
    "D3D_NAME_FINAL_QUAD_EDGE_TESSFACTOR",
    "D3D_NAME_FINAL_QUAD_INSIDE_TESSFACTOR",
    "D3D_NAME_FINAL_TRI_EDGE_TESSFACTOR",
    "D3D_NAME_FINAL_TRI_INSIDE_TESSFACTOR",
    "D3D_NAME_FINAL_LINE_DETAIL_TESSFACTOR",
    "D3D_NAME_FINAL_LINE_DENSITY_TESSFACTOR",
    "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", // 17-63 unused
    "D3D_NAME_TARGET",
    "D3D_NAME_DEPTH",
    "D3D_NAME_COVERAGE",
    "D3D_NAME_DEPTH_GREATER_EQUAL",
    "D3D_NAME_DEPTH_LESS_EQUAL"
};

eastl::string const HLSLParameter::msComponentType[] =
{
    "D3D_REGISTER_COMPONENT_UNKNOWN",
    "D3D_REGISTER_COMPONENT_UINT32",
    "D3D_REGISTER_COMPONENT_SINT32",
    "D3D_REGISTER_COMPONENT_FLOAT32"
};

eastl::string const HLSLParameter::msMinPrecision[] =
{
    "D3D_MIN_PRECISION_DEFAULT",
    "D3D_MIN_PRECISION_FLOAT_16",
    "D3D_MIN_PRECISION_FLOAT_2_8",
    "D3D_MIN_PRECISION_RESERVED",
    "D3D_MIN_PRECISION_SINT_16",
    "D3D_MIN_PRECISION_UINT_16",
    "D3D_MIN_PRECISION_ANY_16",
    "D3D_MIN_PRECISION_ANY_10"
};
