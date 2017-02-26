// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef FONT_H
#define FONT_H

#include "Graphic/Resource/Buffer/IndexBuffer.h"
#include "Graphic/Resource/Buffer/VertexBuffer.h"

#include "Graphic/Effect/TextEffect.h"

class GRAPHIC_ITEM Font
{
public:
    // Construction.
    Font(eastl::shared_ptr<ProgramFactory> const& factory, unsigned int width, unsigned int height,
        char const* texels, float const* characterData, unsigned int maxMessageLength);

    // Member access.
    inline eastl::shared_ptr<VertexBuffer> const& GetVertexBuffer() const;
    inline eastl::shared_ptr<IndexBuffer> const& GetIndexBuffer() const;
    inline eastl::shared_ptr<TextEffect> const& GetTextEffect() const;

    // Populate the vertex buffer for the specified string.
    void Typeset(int viewportWidth, int viewportHeight, int x, int y,
		Vector4<float> const& color, eastl::string const& message) const;

protected:
    unsigned int mMaxMessageLength;
	eastl::shared_ptr<VertexBuffer> mVertexBuffer;
	eastl::shared_ptr<IndexBuffer> mIndexBuffer;
	eastl::shared_ptr<Texture2> mTexture;
	eastl::shared_ptr<TextEffect> mTextEffect;
    float mCharacterData[257];

    // Shader source code as strings.
    static eastl::string const msGLSLVSSource;
    static eastl::string const msGLSLPSSource;
    static eastl::string const msHLSLSource;
    static eastl::string const* msVSSource[ProgramFactory::PF_NUM_API];
    static eastl::string const* msPSSource[ProgramFactory::PF_NUM_API];
};


inline eastl::shared_ptr<VertexBuffer> const& Font::GetVertexBuffer() const
{
    return mVertexBuffer;
}

inline eastl::shared_ptr<IndexBuffer> const& Font::GetIndexBuffer() const
{
    return mIndexBuffer;
}

inline eastl::shared_ptr<TextEffect> const& Font::GetTextEffect() const
{
    return mTextEffect;
}

#endif
