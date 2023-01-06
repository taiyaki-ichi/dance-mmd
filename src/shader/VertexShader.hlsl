#include"Header.hlsli"

VSOutput main(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	VSOutput output;
	output.pos = mul(cameraData.viewProj, mul(modelData.world, pos));
	output.normal = mul(modelData.world, pos);
	output.uv = uv;

	return output;
}