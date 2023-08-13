#include"ShapeHeader.hlsli"

VSOutput main(float4 pos : POSITION, float4 normal : NORMAL, uint instanceId : SV_InstanceId)
{
	VSOutput output;
	output.position = mul(cameraData.viewProj, mul(shapeData[instanceId].transform, pos));
	normal.w = 0.f;
	output.normal = normalize(mul(shapeData[instanceId].transform, normal));
	output.instanceId = instanceId;

	return output;
}