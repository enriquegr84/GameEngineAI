// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/09/12)

#include "Core/Logger/Logger.h"

#include "HLSLShaderType.h"

HLSLShaderType::HLSLShaderType()
    :
    mName("")
{
}

void HLSLShaderType::SetDescription(D3D_SHADER_TYPE_DESC const& desc)
{
    mDesc.varClass = desc.Class;
    mDesc.varType = desc.Type;
    mDesc.numRows = desc.Rows;
    mDesc.numColumns = desc.Columns;
    mDesc.numElements = desc.Elements;
    mDesc.numChildren = desc.Members;
    mDesc.offset = desc.Offset;
    mDesc.typeName = eastl::string(desc.Name ? desc.Name : "");

    if (desc.Members > 0)
    {
        mChildren.resize(desc.Members);
    }
    else
    {
        mChildren.clear();
    }
}

void HLSLShaderType::SetName(eastl::string const& name)
{
    mName = name;
}

HLSLShaderType& HLSLShaderType::GetChild(unsigned int i)
{
    LogAssert(i < mDesc.numChildren, "Invalid index.");
    return mChildren[i];
}

HLSLShaderType const& HLSLShaderType::GetChild(unsigned int i) const
{
    LogAssert(i < mDesc.numChildren, "Invalid index.");
    return mChildren[i];
}

eastl::string const& HLSLShaderType::GetName() const
{
    return mName;
}

D3D_SHADER_VARIABLE_CLASS HLSLShaderType::GetClass() const
{
    return mDesc.varClass;
}

D3D_SHADER_VARIABLE_TYPE HLSLShaderType::GetType() const
{
    return mDesc.varType;
}

unsigned int HLSLShaderType::GetNumRows() const
{
    return mDesc.numRows;
}

unsigned int HLSLShaderType::GetNumColumns() const
{
    return mDesc.numColumns;
}

unsigned int HLSLShaderType::GetNumElements() const
{
    return mDesc.numElements;
}

unsigned int HLSLShaderType::GetNumChildren() const
{
    return mDesc.numChildren;
}

unsigned int HLSLShaderType::GetOffset() const
{
    return mDesc.offset;
}

eastl::string const& HLSLShaderType::GetTypeName() const
{
    return mDesc.typeName;
}

eastl::vector<HLSLShaderType> const& HLSLShaderType::GetChildren() const
{
    return mChildren;
}

void HLSLShaderType::Print(std::ofstream& output, int indent) const
{
    std::string prefix = "";
    for (int i = 0; i < indent; ++i)
    {
        prefix += "    ";
    }

    output << prefix << "name = " << mName.c_str() << std::endl;
    output << prefix << "variable class = " << msVarClass[mDesc.varClass].c_str()
        << std::endl;
    output << prefix << "variable type = " << msVarType[mDesc.varType].c_str()
        << std::endl;
    output << prefix << "rows = " << mDesc.numRows << std::endl;
    output << prefix << "columns = " << mDesc.numColumns << std::endl;
    output << prefix << "elements = " << mDesc.numElements << std::endl;
    output << prefix << "children = " << mDesc.numChildren << std::endl;
    output << prefix << "offset = " << mDesc.offset << std::endl;
    output << prefix << "type name = " << mDesc.typeName.c_str() << std::endl;

    ++indent;
    for (auto const& child : mChildren)
    {
        child.Print(output, indent);
    }
}


eastl::string const HLSLShaderType::msVarClass[] =
{
    "D3D_SVC_SCALAR",
    "D3D_SVC_VECTOR",
    "D3D_SVC_MATRIX_ROWS",
    "D3D_SVC_MATRIX_COLUMNS",
    "D3D_SVC_OBJECT",
    "D3D_SVC_STRUCT",
    "D3D_SVC_INTERFACE_CLASS",
    "D3D_SVC_INTERFACE_POINTER"
};

eastl::string const HLSLShaderType::msVarType[] =
{
    "D3D_SVT_VOID",
    "D3D_SVT_BOOL",
    "D3D_SVT_INT",
    "D3D_SVT_FLOAT",
    "D3D_SVT_STRING",
    "D3D_SVT_TEXTURE",
    "D3D_SVT_TEXTURE1D",
    "D3D_SVT_TEXTURE2D",
    "D3D_SVT_TEXTURE3D",
    "D3D_SVT_TEXTURECUBE",
    "D3D_SVT_SAMPLER",
    "D3D_SVT_SAMPLER1D",
    "D3D_SVT_SAMPLER2D",
    "D3D_SVT_SAMPLER3D",
    "D3D_SVT_SAMPLERCUBE",
    "D3D_SVT_PIXELSHADER",
    "D3D_SVT_VERTEXSHADER",
    "D3D_SVT_PIXELFRAGMENT",
    "D3D_SVT_VERTEXFRAGMENT",
    "D3D_SVT_UINT",
    "D3D_SVT_UINT8",
    "D3D_SVT_GEOMETRYSHADER",
    "D3D_SVT_RASTERIZER",
    "D3D_SVT_DEPTHSTENCIL",
    "D3D_SVT_BLEND",
    "D3D_SVT_BUFFER",
    "D3D_SVT_CBUFFER",
    "D3D_SVT_TBUFFER",
    "D3D_SVT_TEXTURE1DARRAY",
    "D3D_SVT_TEXTURE2DARRAY",
    "D3D_SVT_RENDERTARGETVIEW",
    "D3D_SVT_DEPTHSTENCILVIEW",
    "D3D_SVT_TEXTURE2DMS",
    "D3D_SVT_TEXTURE2DMSARRAY",
    "D3D_SVT_TEXTURECUBEARRAY",
    "D3D_SVT_HULLSHADER",
    "D3D_SVT_DOMAINSHADER",
    "D3D_SVT_INTERFACE_POINTER",
    "D3D_SVT_COMPUTESHADER",
    "D3D_SVT_DOUBLE",
    "D3D_SVT_RWTEXTURE1D",
    "D3D_SVT_RWTEXTURE1DARRAY",
    "D3D_SVT_RWTEXTURE2D",
    "D3D_SVT_RWTEXTURE2DARRAY",
    "D3D_SVT_RWTEXTURE3D",
    "D3D_SVT_RWBUFFER",
    "D3D_SVT_BYTEADDRESS_BUFFER",
    "D3D_SVT_RWBYTEADDRESS_BUFFER",
    "D3D_SVT_STRUCTURED_BUFFER",
    "D3D_SVT_RWSTRUCTURED_BUFFER",
    "D3D_SVT_APPEND_STRUCTURED_BUFFER",
    "D3D_SVT_CONSUME_STRUCTURED_BUFFER",
    "D3D_SVT_MIN8FLOAT",
    "D3D_SVT_MIN10FLOAT",
    "D3D_SVT_MIN16FLOAT",
    "D3D_SVT_MIN12INT",
    "D3D_SVT_MIN16INT",
    "D3D_SVT_MIN16UINT"
};
