#include"Header.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
	float diffuse = saturate(dot(-directionLightData.dir, input.normal.xyz));
	float4 texColor = tex.Sample(smp, input.uv);
	texColor.a = 1.f;
	return texColor;
}