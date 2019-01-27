// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

cbuffer PVWMatrix
{
    float4x4 pvwMatrix;
};

struct VS_INPUT
{
    float3 modelPosition : POSITION;
    float3 modelNormal : NORMAL;
	float2 modelTCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float3 vertexPosition : TEXCOORD0;
    float3 vertexNormal : TEXCOORD1;
	float2 vertexTCoord : TEXCOORD2;
    float4 clipPosition : SV_POSITION;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;

    output.vertexPosition = input.modelPosition;
    output.vertexNormal = input.modelNormal;
	output.vertexTCoord = input.modelTCoord;
#if GE_USE_MAT_VEC
    output.clipPosition = mul(pvwMatrix, float4(input.modelPosition, 1.0f));
#else
    output.clipPosition = mul(float4(input.modelPosition, 1.0f), pvwMatrix);
#endif
    return output;
}

cbuffer Material
{
    float4 materialEmissive;
    float4 materialAmbient;
    float4 materialDiffuse;
    float4 materialSpecular;
};

cbuffer Lighting
{
    float4 lightingAmbient;
    float4 lightingDiffuse;
    float4 lightingSpecular;
    float4 lightingSpotCutoff;
    float4 lightingAttenuation;
};

cbuffer LightCameraGeometry
{
    float4 lightModelPosition;
    float4 lightModelDirection;
    float4 cameraModelPosition;
};

Texture2D<float4> baseTexture;
SamplerState baseSampler;

struct PS_INPUT
{
    float3 vertexPosition : TEXCOORD0;
    float3 vertexNormal : TEXCOORD1;
	float2 vertexTCoord : TEXCOORD2;
};

struct PS_OUTPUT
{
    float4 pixelColor0 : SV_TARGET0;
};

PS_OUTPUT PSMain(PS_INPUT input)
{
    PS_OUTPUT output;

    float4 lighting;
    float3 normal = normalize(input.vertexNormal);
    float3 modelLightDiff = input.vertexPosition - lightModelPosition.xyz;
    float3 vertexDirection = normalize(modelLightDiff);
    float vertexCosAngle = dot(lightModelDirection.xyz, vertexDirection);
    if (vertexCosAngle >= lightingSpotCutoff.y)
    {
        float NDotL = -dot(normal, vertexDirection);
        float3 viewVector = normalize(cameraModelPosition.xyz - input.vertexPosition);
        float3 halfVector = normalize(viewVector - vertexDirection);
        float NDotH = dot(normal, halfVector);
        lighting = lit(NDotL, NDotH, materialSpecular.a);
        lighting.w = pow(abs(vertexCosAngle), lightingSpotCutoff.w);
    }
    else
    {
        lighting = float4(1.0f, 0.0f, 0.0f, 0.0f);
    }
	// Compute the lighting color.
	float3 lightingColor = materialAmbient.rgb * lightingAmbient.rgb + lighting.w * (
		lighting.y * materialDiffuse.rgb * lightingDiffuse.rgb +
		lighting.z * materialSpecular.rgb * lightingSpecular.rgb);

    // Compute the distance-based attenuation.
    float distance = length(modelLightDiff);
    float attenuation = lightingAttenuation.w / (lightingAttenuation.x + distance *
        (lightingAttenuation.y + distance * lightingAttenuation.z));

	float4 textureColor = baseTexture.Sample(baseSampler, input.vertexTCoord);

    // Compute the pixel color.
	float3 color = lightingColor * textureColor.rgb;
    output.pixelColor0.rgb = materialEmissive.rgb + attenuation * color;
    output.pixelColor0.a = materialDiffuse.a * textureColor.a;
    return output;
}
