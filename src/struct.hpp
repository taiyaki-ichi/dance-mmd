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

// info�̏��͓��Ɏg�p���Ȃ��̂ł���ł����[
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


// �}�e���A��
// �V�F�[�_�ɓn���p
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
	// �����Ȃ�
	none,

	// ��Z
	sph,

	// ���Z
	spa,

	// �T�u�e�N�X�`��
	// �ǉ�UV1��x,y��UV�Q�Ƃ��Ēʏ�e�N�X�`���`����s��
	subtexture,
};

enum class toon_type
{
	// ���L
	shared,

	// ��
	unshared,
};

// �}�e���A���̃e�N�X�`���̏��
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
	// �ڑ���@�����@
	// false: ���W�I�t�Z�b�g�Ŏw��
	// true: �{�[���Ŏw��
	access_point = 0,

	// ��]�\���ǂ���
	rotatable = 1,

	// �ړ��\���ǂ���
	movable = 2,

	// �\��
	// WARNING; �\���\���ǂ����Ƃ����Ӗ���?
	display = 3,

	// ����\���ǂ���
	operable = 4,

	// IK�g�����ǂ���
	ik = 5,

	// ���[�J���t�^
	// false: ���[�U�[�ό`�l�^IK�����N�^���d�t�^
	// true: �e�̃��[�J���ό`��
	local_grant = 7,

	// ��]�t�^
	rotation_grant = 8,

	// �ړ��t�^
	move_grant = 9,

	// ���Œ肩�ǂ���
	fix_axis = 10,

	// ���[�J�������ǂ���
	local_axis = 11,

	// ������ό`���ǂ���
	post_physical_deformation = 12,

	// �O���e�ό`���ǂ���
	external_parent_deformation = 13,

};


struct ik_link {
	// �����N���Ă���{�[��
	std::size_t bone;

	// �p�x�������s���ꍇ�̍ŏ��A�ő�̊p�x�̐���
	std::optional<std::pair<XMFLOAT3, XMFLOAT3>> min_max_angle_limit;
};


struct model_bone_data
{
	// ���O
	std::wstring name;

	// �ʒu
	XMFLOAT3 position;
	// �e�{�[���̃C���f�b�N�X
	std::size_t parent_index;

	std::bitset<16> bone_flag_bits;

	// bone_flag_bits[rotation_grant]=true�܂��� bone_flag_bits[move_grant]=true�̂Ƃ��g�p
	// �t�^�Ώۂ̃{�[��
	std::size_t grant_index;
	// �t�^��
	float grant_rate;

	// bone_flag_bits[ik]=true�̎��Ɏg�p
	//IK�̃^�[�Q�b�g�̃{�[��
	std::size_t ik_target_bone;
	// IK�̃��[�v��
	std::int32_t ik_roop_number;
	// IK�̃��[�v���s���ۂ�1�񓖂���̐����p�x�i���W�A���j
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


	// �Q�l: http://atupdate.web.fc2.com/vmd_format.htm
};

struct morph_motion_data
{
	int frame_num{};
	float weight{};
};

// �{�[�����v�Z���鎞�Ɏg��
struct bone_data
{
	// ��]��\���N�I�[�^�j�I��
	XMVECTOR rotation;

	// ���s�ړ���\���x�N�^
	XMVECTOR transform;

	float _pad0;

	// ���[�J���̍��W�����[���h�ɕϊ�����
	// �e�{�[���̉�]�A���s�ړ��̏��
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
