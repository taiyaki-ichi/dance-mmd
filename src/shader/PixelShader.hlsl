#include"Header.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
	// テクスチャの色
	float4 textureColor = tex.Sample(smp, input.uv);

	// 光の反射のベクトル
	float3 lightReflection = normalize(reflect(directionLightData.direction, input.normal.xyz));

	float4 diffuse = saturate(dot(directionLightData.direction, input.normal.xyz)) * materialData.diffuse * textureColor;
	float4 ambient = float4(textureColor * materialData.ambient, 1);

	return diffuse + ambient;
}