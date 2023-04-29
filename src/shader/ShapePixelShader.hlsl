#include"ShapeHeader.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
	float3 lightDir = float3(1.f,1.f,-1.f);

	float diffuseBias = saturate(dot(lightDir, input.normal.xyz));
	float3 diffuse = shapeData[input.instanceId].color * 0.8f * diffuseBias;
	float3 ambient = shapeData[input.instanceId].color * 0.2f;
	return float4(ambient + diffuse, 1.f);
}