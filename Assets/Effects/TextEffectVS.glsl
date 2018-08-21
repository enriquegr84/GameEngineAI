// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

uniform Translate
{
    vec2 translate;
};

layout(location = 0) in vec2 modelPosition;
layout(location = 1) in vec2 modelTCoord;
layout(location = 0) out vec2 vertexTCoord;

void main()
{
    vertexTCoord = modelTCoord;
    gl_Position.x = 2.0f * modelPosition.x - 1.0f + 2.0f * translate.x;
    gl_Position.y = 2.0f * modelPosition.y - 1.0f + 2.0f * translate.y;
    gl_Position.z = -1.0f;
    gl_Position.w = 1.0f;
}
