#pragma once
#include<cstdint>
#include<DirectXMath.h>
#include"parameter.hpp"
#include<optional>
#include<bitset>
#include<Windows.h>


using namespace DirectX;

struct model_header_data
{
	std::uint8_t add_uv_number{};
	std::uint8_t vertex_index_size{};
	std::uint8_t texture_index_size{};
	std::uint8_t material_index_size{};
	std::uint8_t bone_index_size{};
	std::uint8_t morph_index_size{};
	std::uint8_t rigid_body_index_size{};
};

// infoの情報は特に使用しないのでこれでおけー
struct model_info_data {};

struct model_vertex_data
{
	XMFLOAT3 position{};
	XMFLOAT3 normal{};
	XMFLOAT2 uv{};
	std::array<std::uint32_t, 4> bone_index{};
	std::array<float, 4> bone_weight{};
};

using index = std::uint32_t;


// マテリアル
// シェーダに渡す用
struct material_data
{
	XMFLOAT4 diffuse{};
	XMFLOAT3 specular{};
	float specularity{};
	XMFLOAT3 ambient{};
	float _pad0{};
};

enum class sphere_mode
{
	// 何もなし
	none,

	// 乗算
	sph,

	// 加算
	spa,

	// サブテクスチャ
	// 追加UV1のx,yをUV参照して通常テクスチャ描画を行う
	subtexture,
};

enum class toon_type
{
	// 共有
	shared,

	// 個別
	unshared,
};

// マテリアルのテクスチャの情報
struct material_data_2
{
	std::size_t texture_index{};
	sphere_mode sphere_mode{};
	std::size_t sphere_texture_index{};
	toon_type toon_type{};
	std::size_t toon_texture{};
	std::size_t vertex_number{};
};

enum class bone_flag
{
	// 接続先法事方法
	// false: 座標オフセットで指定
	// true: ボーンで指定
	access_point = 0,

	// 回転可能かどうか
	rotatable = 1,

	// 移動可能かどうか
	movable = 2,

	// 表示
	// WARNING; 表示可能かどうかという意味か?
	display = 3,

	// 操作可能かどうか
	operable = 4,

	// IK使うかどうか
	ik = 5,

	// ローカル付与
	// false: ユーザー変形値／IKリンク／多重付与
	// true: 親のローカル変形量
	local_grant = 7,

	// 回転付与
	rotation_grant = 8,

	// 移動付与
	move_grant = 9,

	// 軸固定かどうか
	fix_axis = 10,

	// ローカル軸かどうか
	local_axis = 11,

	// 物理後変形かどうか
	post_physical_deformation = 12,

	// 外部親変形かどうか
	external_parent_deformation = 13,

};


struct ik_link {
	// リンクしているボーン
	std::size_t bone;

	// 角度制限を行う場合の最小、最大の角度の制限
	std::optional<std::pair<XMFLOAT3, XMFLOAT3>> min_max_angle_limit;
};


struct model_bone_data
{
	// 名前
	std::wstring name;

	// 位置
	XMFLOAT3 position;
	// 親ボーンのインデックス
	std::size_t parent_index;

	std::bitset<16> bone_flag_bits;

	// bone_flag_bits[rotation_grant]=trueまたは bone_flag_bits[move_grant]=trueのとき使用
	// 付与対象のボーン
	std::size_t grant_index;
	// 付与率
	float grant_rate;

	// bone_flag_bits[ik]=trueの時に使用
	//IKのターゲットのボーン
	std::size_t ik_target_bone;
	// IKのループ回数
	std::int32_t ik_roop_number;
	// IKのループを行う際の1回当たりの制限角度（ラジアン）
	float ik_rook_angle;
	// iklink
	std::vector<ik_link> ik_link;
};

struct vmd_header
{
	std::size_t frame_data_num{};
};

struct vmd_frame_data
{
	std::wstring name{};
	std::size_t frame_num{};
	XMFLOAT3 transform{};
	XMFLOAT4 quaternion{};
	std::array<char, 64> complement_parameter{};
};

struct vmd_morph_data
{
	std::wstring name{};
	std::size_t frame_num{};
	float weight{};
};

struct vpd_data
{
	std::wstring name{};
	XMFLOAT3 transform{};
	XMFLOAT4 quaternion{};
};


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


struct bone_motion_data
{
	int frame_num;
	XMFLOAT3 transform;
	XMFLOAT4 quaternion;

	std::array<char, 2> r_a;
	std::array<char, 2> r_b;

	std::array<char, 2> x_a;
	std::array<char, 2> x_b;

	std::array<char, 2> y_a;
	std::array<char, 2> y_b;

	std::array<char, 2> z_a;
	std::array<char, 2> z_b;


	// 参考: http://atupdate.web.fc2.com/vmd_format.htm
};

struct morph_motion_data
{
	int frame_num{};
	float weight{};
};

// ボーンを計算する時に使う
struct bone_data
{
	// 回転を表すクオータニオン
	XMVECTOR rotation;

	// 並行移動を表すベクタ
	XMVECTOR transform;

	float _pad0;

	// ローカルの座標をワールドに変換する
	// 親ボーンの回転、並行移動の情報
	XMMATRIX to_world;
};

struct vertex_morph_data
{
	XMFLOAT3 offset{};
	std::int32_t index{};
};

struct vertex_morph
{
	std::size_t morph_index{};

	std::wstring name{};
	std::wstring english_name{};

	std::vector<vertex_morph_data> data{};
};

struct group_morph_data
{
	std::size_t index{};
	float morph_factor{};
};

struct group_morph
{
	std::size_t morph_index{};

	std::wstring name{};
	std::wstring english_name{};

	std::vector<group_morph_data> data{};
};
