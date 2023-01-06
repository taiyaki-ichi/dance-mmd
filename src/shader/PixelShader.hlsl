#include"Header.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
	float diffuse = saturate(dot(-directionLightData.dir, input.normal.xyz));


	return float4(0.8f, 0.8f, 0.8f, 1.0f) * diffuse + float4(0.2f, 0.2f, 0.2f, 1.0f);
}