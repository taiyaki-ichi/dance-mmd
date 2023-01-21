#pragma once
#include<cstdint>
#include<DirectXMath.h>
#include"parameter.hpp"

using namespace DirectX;

struct vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	std::uint32_t bone_index[4];
	float bone_weight[4];
};

using index = std::uint32_t;

struct model_data
{
	XMMATRIX world;
	std::array<XMMATRIX, MAX_BONE_NUM> bone;
};

struct camera_data
{
	XMMATRIX view;
	XMMATRIX viewInv;
	XMMATRIX proj;
	XMMATRIX projInv;
	XMMATRIX viewProj;
	XMMATRIX viewProjInv;
	float cameraNear;
	float cameraFar;
	float screenWidth;
	float screenHeight;
	XMFLOAT3 eyePos;
	float _pad0;
};

struct direction_light_data
{
	XMFLOAT3 dir;
	float _pad0;
	XMFLOAT3 color;
	float _pad1;
};


struct material_data
{
	XMFLOAT4 diffuse;
	XMFLOAT3 specular;
	float specularity;
	XMFLOAT3 ambient;
	float _pad0;
};

struct bone_motion_data
{
	int frame_num;
	XMFLOAT3 transform;
	XMFLOAT4 quaternion;

	// ï‚äÆÉpÉâÉÅÅ[É^Ç…Ç¬Ç¢ÇƒÇÕÇ‹ÇΩÇ†Ç∆Ç≈
};