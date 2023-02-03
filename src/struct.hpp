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

	std::array<char, 2> r_a;
	std::array<char, 2> r_b;

	std::array<char,2> x_a;
	std::array<char, 2> x_b;

	std::array<char, 2> y_a;
	std::array<char, 2> y_b;

	std::array<char, 2> z_a;
	std::array<char, 2> z_b;


	// 参考: http://atupdate.web.fc2.com/vmd_format.htm
};

// ボーンを計算する時に使う
struct bone_data
{
	// 回転を表す行列
	XMMATRIX rotation;

	// 並行移動を表すベクタ
	XMFLOAT3 transform;

	float _pad0;

	// ローカルの座標をワールドに変換する
	// 親ボーンの回転、並行移動の情報
	XMMATRIX to_world;
};
