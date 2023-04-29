
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

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	uint instanceId : INSTANCE_ID;
};

#define MAX_SHAPE_NUM 256

struct shape_data
{
	matrix transform;
	float3 color;
};


cbuffer CameraDataConstantBuffer : register(b0)
{
	CameraData cameraData;
}

cbuffer ShapeDataConstantBuffer : register(b1)
{
	shape_data shapeData[MAX_SHAPE_NUM];
}