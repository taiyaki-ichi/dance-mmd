
struct ModelData
{
	matrix world;
};

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

struct DirectionLightData
{
	float3 direction;
	float3 color;
};

struct MaterialData
{
	float4 diffuse;
	float3 specular;
	float specularity;
	float3 ambient;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	// ビュー空間の法線
	float4 vnormal : VNORMAL;
	float2 uv : TEXCOORD;
	// 対象頂点への視点からのベクトル
	float3 ray:VECTOR;
};

cbuffer ModelDataConstantBuffer : register(b0)
{
	ModelData modelData;
}

cbuffer CameraDataConstantBuffer : register(b1)
{
	CameraData cameraData;
}

cbuffer DirectionLightDataConstantBuffer : register(b2)
{
	DirectionLightData directionLightData;
}

cbuffer MaterialDataConstantBuffer : register(b3)
{
	MaterialData materialData;
}

// 通常のテクスチャ
Texture2D<float4> tex : register(t0);

// 乗算スフィアマップ
Texture2D<float4> sph:register(t1);

// 乗算スフィアマップ
Texture2D<float4> spa:register(t2);

// トゥーン
Texture2D<float4> toon:register(t3);

SamplerState smp:register(s0);

// トゥーン用のサンプラ
SamplerState smpToon:register(s1);