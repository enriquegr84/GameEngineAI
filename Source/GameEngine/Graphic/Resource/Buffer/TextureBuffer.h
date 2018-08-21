// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2016/08/29)

#ifndef TEXTUREBUFFER_H
#define TEXTUREBUFFER_H

#include "Graphic/Resource/MemberLayout.h"
#include "Graphic/Resource/Buffer/Buffer.h"
#include "Graphic/Resource/Texture/Texture.h"

class GRAPHIC_ITEM TextureBuffer : public Buffer
{
public:
    // Construction.  The HLSL shader contains
    //   tbuffer MyTBuffer { type myArray[numElements]; }
    // where 'type' is a native type such as 'float4' and the texture buffer
    // 'format' specifies how the type is interpreted.  The 'numElements' of
    // the constructor must match that of myArray[].  Effectively, the
    // tbuffer is a 1D texture.  If you want to update the contents of the
    // texture buffer at run time, much like you update a constant buffer,
    // set 'allowDynamicUpdate' to 'true'; otherwise, the buffer is immutable.
    TextureBuffer(DFType format, unsigned int numElements, bool allowDynamicUpdate);

    DFType GetFormat() const;

    // Access to texture buffer members using the layout of a shader
    // program itself is allowed as long as you have attached the constant
    // buffer to a shader first.
    //   eastl::shared_ptr<VertexShader> vshader = <some shader>;
    //   eastl::shared_ptr<TextureBuffer> tbuffer = <buffer for the shader>;
    //   vshader->Set("MyTBuffer", tbuffer);
    // Now you can use SetMember/GetMember calls successfully.  In these
    // calls, you are required to specify the correct type T for the member.
    // No checking is performed for the size of the input; i.e., too large a
    // 'value' will cause a memory overwrite within the buffer.  The code
    // does test to ensure that no overwrite occurs outside the buffer.

    inline void SetLayout(eastl::vector<MemberLayout> const& layout);
    inline eastl::vector<MemberLayout> const& GetLayout() const;

    // Test for existence of a member with the specified name.
    bool HasMember(eastl::string const& name) const;

    // Set or get a non-array member.
    template <typename T>
    bool SetMember(eastl::string const& name, T const& value);

    template <typename T>
    bool GetMember(eastl::string const& name, T& value) const;

    // Set or get an array member.
    template <typename T>
    bool SetMember(eastl::string const& name, unsigned int index, T const& value);

    template <typename T>
    bool GetMember(eastl::string const& name, unsigned int index, T& value) const;

protected:
    DFType mFormat;
	eastl::vector<MemberLayout> mLayout;

public:
    // For use by the Shader class for storing reflection information.
    static int const mShaderDataLookup = 1;
};


inline void TextureBuffer::SetLayout(eastl::vector<MemberLayout> const& layout)
{
    mLayout = layout;
}

inline eastl::vector<MemberLayout> const& TextureBuffer::GetLayout() const
{
    return mLayout;
}

template <typename T>
bool TextureBuffer::SetMember(eastl::string const& name, T const& value)
{
    auto iter = eastl::find_if(mLayout.begin(), mLayout.end(),
        [&name](MemberLayout const& item){ return name == item.name; });

    if (iter == mLayout.end())
    {
        LogError("Failed to find member name " + name + ".");
        return false;
    }

    if (iter->numElements > 0)
    {
        LogError("Member is an array, use SetMember(name,index,value).");
        return false;
    }

    if (iter->offset + sizeof(T) > mNumBytes)
    {
        LogError("Writing will access memory outside the buffer.");
        return false;
    }

    T* target = reinterpret_cast<T*>(mData + iter->offset);
    *target = value;
    return true;
}

template <typename T>
bool TextureBuffer::GetMember(eastl::string const& name, T& value) const
{
    auto iter = eastl::find_if(mLayout.begin(), mLayout.end(),
        [&name](MemberLayout const& item){ return name == item.name; });

    if (iter == mLayout.end())
    {
        LogError("Failed to find member name " + name + ".");
        return false;
    }

    if (iter->numElements > 0)
    {
        LogError("Member is an array, use GetMember(name,index,value).");
        return false;
    }

    if (iter->offset + sizeof(T) > mNumBytes)
    {
        LogError("Reading will access memory outside the buffer.");
        return false;
    }

    T* target = reinterpret_cast<T*>(mData + iter->offset);
    value = *target;
    return true;
}

template <typename T>
bool TextureBuffer::SetMember(eastl::string const& name, unsigned int index, T const& value)
{
    auto iter = eastl::find_if(mLayout.begin(), mLayout.end(),
        [&name](MemberLayout const& item){ return name == item.name; });

    if (iter == mLayout.end())
    {
        LogError("Failed to find member name " + name + ".");
        return false;
    }

    if (iter->numElements == 0)
    {
        LogError("Member is a singleton, use SetMember(name,value).");
        return false;
    }

    if (index >= iter->numElements)
    {
        LogError("Index is out of range for the member array.");
        return false;
    }

    if (iter->offset + (index + 1)*sizeof(T) > mNumBytes)
    {
        LogError("Writing will access memory outside the buffer.");
        return false;
    }

    T* target = reinterpret_cast<T*>(mData + iter->offset + index*sizeof(T));
    *target = value;
    return true;
}

template <typename T>
bool TextureBuffer::GetMember(eastl::string const& name, unsigned int index, T& value) const
{
    auto iter = eastl::find_if(mLayout.begin(), mLayout.end(),
        [&name](MemberLayout const& item){ return name == item.name; });

    if (iter == mLayout.end())
    {
        LogError("Failed to find member name " + name + ".");
        return false;
    }

    if (iter->numElements == 0)
    {
        LogError("Member is a singleton, use GetMember(name,value).");
        return false;
    }

    if (index >= iter->numElements)
    {
        LogError("Index is out of range for the member array.");
        return false;
    }

    if (iter->offset + (index + 1)*sizeof(T) > mNumBytes)
    {
        LogError("Reading will access memory outside the buffer.");
        return false;
    }

    T* target = reinterpret_cast<T*>(mData + iter->offset + index*sizeof(T));
    value = *target;
    return true;
}

#endif
