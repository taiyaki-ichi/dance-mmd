#include"Header.hlsli"

VSOutput main(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	VSOutput output;
	output.pos = mul(cameraData.viewProj, mul(modelData.world, pos));
	normal.w = 0.f;
	output.normal = mul(modelData.world, normal);
	output.uv = uv;
	output.ray = normalize(mul(modelData.world, pos) - cameraData.eyePos);

	return output;
}