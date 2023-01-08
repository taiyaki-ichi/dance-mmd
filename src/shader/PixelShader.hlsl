#include"Header.hlsli"

float4 main(VSOutput input) : SV_TARGET
{
	// スフィアマップ用のUV
	float2 sphereUV = (input.vnormal.xy + float2(1.f,-1.f)) * float2(0.5f,-0.5f);

	// テクスチャの色
	float4 textureColor = tex.Sample(smp, input.uv) * sph.Sample(smp, sphereUV) + spa.Sample(smp, sphereUV);

	// 光の反射のベクトル
	float3 lightReflection = normalize(reflect(directionLightData.direction, input.normal.xyz));

	float diffuseBias = saturate(dot(directionLightData.direction, input.normal.xyz));
	float4 toonDiffuse = toon.Sample(smpToon, float2(0.5f, 1.f - diffuseBias)) * textureColor;
	float4 specular = materialData.specularity <= 0.f ? float4(0.f, 0.f, 0.f, 0.f) : pow(saturate(dot(lightReflection, -input.ray)), materialData.specularity) * float4(materialData.specular, 1.f) * textureColor;
	float4 ambient = textureColor * float4(materialData.ambient, 1.f);

	return max(toonDiffuse + specular, ambient);
}