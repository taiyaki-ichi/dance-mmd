#include"Header.hlsli"

VSOutput main(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, uint4 boneIndex : BONEINDEX, float4 boneWeight : BONEWEIGHT)
{
	matrix boneMatrix =
		modelData.bone[boneIndex[0]] * boneWeight[0] +
		modelData.bone[boneIndex[1]] * boneWeight[1] +
		modelData.bone[boneIndex[2]] * boneWeight[2] +
		modelData.bone[boneIndex[3]] * boneWeight[3];

	pos = mul(boneMatrix, pos);

	VSOutput output;
	output.pos = mul(cameraData.viewProj, mul(modelData.world, pos));
	normal.w = 0.f;
	output.normal = mul(modelData.world, normal);
	output.vnormal = mul(cameraData.view, output.normal);
	output.uv = uv;
	output.ray = normalize(mul(modelData.world, pos) - cameraData.eyePos);

	return output;
}