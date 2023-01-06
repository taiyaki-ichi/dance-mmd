
struct ModelData
{
	matrix world;
};

struct CameraData
{
	matrix view;
	matrix viewInv;
	matrix proj;
	matrix projInv;
	matrix viewProj;
	matrix viewProjInv;
	float cameraNear;
	float cameraFar;
	float screenWidth;
	float screenHeight;
	float3 eyePos;
};

struct DirectionLightData
{
	float3 dir;
	float3 color;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

cbuffer ModelDataConstantBuffer : register(b0)
{
	ModelData modelData;
}

cbuffer CameraDataConstantBuffer : register(b1)
{
	CameraData cameraData;
}

cbuffer DirectionLightDataConstantBuffer : register(b2)
{
	DirectionLightData directionLightData;
}

