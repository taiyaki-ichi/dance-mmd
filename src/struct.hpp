#pragma once
#include<cstdint>
#include<DirectXMath.h>
#include"parameter.hpp"
#include"../external/mmd-loader/mmdl/generics_type.hpp"
#include<optional>
#include<bitset>

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



template<>
struct mmdl::pmx_header_traits<model_header_data>
{
	static model_header_data construct(pmx_header_buffer const& buffer)
	{
		return model_header_data{
			.add_uv_number = buffer.additional_uv_num,
			.vertex_index_size = buffer.vertex_index_size,
			.texture_index_size = buffer.texture_index_size,
			.material_index_size = buffer.material_index_size,
			.bone_index_size = buffer.bone_index_size,
			.morph_index_size = buffer.morph_index_size,
			.rigid_body_index_size = buffer.rigid_body_index_size
		};
	}
};

template<>
struct mmdl::pmx_info_traits<model_info_data>
{
	using char_type = char;

	template<std::size_t CharBufferSize>
	static model_info_data construct(pmx_info_buffer<char_type, CharBufferSize> const&) {
		return {};
	}
};

template<>
struct mmdl::pmx_vertex_traits<std::vector<model_vertex_data>>
{
	// �T�C�Y���w�肵�č\�z
	static std::vector<model_vertex_data> construct(std::size_t size) {
		std::vector<model_vertex_data> result{};
		result.reserve(size);
		return result;
	}

	// �v�f�𖖔��ɒǉ�
	static void emplace_back(std::vector<model_vertex_data>& vertex, pmx_vertex_buffer const& buffer, std::uint8_t additional_uv_num)
	{
		model_vertex_data v{};
		v.position = { buffer.position[0],buffer.position[1] ,buffer.position[2] };
		v.normal = { buffer.normal[0],buffer.normal[1] ,buffer.normal[2] };
		v.uv = { buffer.uv[0],buffer.uv[1] };

		auto weight_sum = buffer.bone_weight[0] + buffer.bone_weight[1] + buffer.bone_weight[2] + buffer.bone_weight[3];

		switch (buffer.weight_type)
		{
			// BDEF1
		case 0:
			v.bone_index[0] = static_cast<std::uint32_t>(buffer.bone_index[0]);
			v.bone_weight[0] = 1.f;
			break;

			// BDEF2
		case 1:
			v.bone_index[0] = static_cast<std::uint32_t>(buffer.bone_index[0]);
			v.bone_index[1] = static_cast<std::uint32_t>(buffer.bone_index[1]);
			v.bone_weight[0] = buffer.bone_weight[0];
			v.bone_weight[1] = 1.f - buffer.bone_weight[0];
			break;

			// BDEF4
		case 2:
			v.bone_index[0] = static_cast<std::uint32_t>(buffer.bone_index[0]);
			v.bone_index[1] = static_cast<std::uint32_t>(buffer.bone_index[1]);
			v.bone_index[2] = static_cast<std::uint32_t>(buffer.bone_index[2]);
			v.bone_index[3] = static_cast<std::uint32_t>(buffer.bone_index[3]);
			v.bone_weight[0] = buffer.bone_weight[0] / weight_sum;
			v.bone_weight[1] = buffer.bone_weight[1] / weight_sum;
			v.bone_weight[2] = buffer.bone_weight[2] / weight_sum;
			v.bone_weight[3] = buffer.bone_weight[3] / weight_sum;
			break;

			// SDEF�͎������Ă��Ȃ��̂ŁA�Ƃ肠����BDEF2�Ƃ��Ȃ���
		case 3:
			v.bone_index[0] = static_cast<std::uint32_t>(buffer.bone_index[0]);
			v.bone_index[1] = static_cast<std::uint32_t>(buffer.bone_index[1]);
			v.bone_weight[0] = buffer.bone_weight[0];
			v.bone_weight[1] = 1.f - buffer.bone_weight[0];
			break;
		}

		vertex.push_back(v);
	}
};

template<>
struct mmdl::pmx_surface_traits<std::vector<index>>
{
	// �T�C�Y���w�肵�č\�z
	static std::vector<index> construct(std::size_t size) {
		std::vector<index> result{};
		result.reserve(size);
		return result;
	}

	// �v�f��ǉ�
	static void emplace_back(std::vector<index>& surface, std::size_t i) {
		surface.push_back(static_cast<index>(i));
	}
};

template<>
struct mmdl::pmx_texture_path_traits<std::vector<std::wstring>>
{
	using char_type = wchar_t;

	// �T�C�Y���w�肵�č\�z
	static std::vector<std::wstring> construct(std::size_t size)
	{
		std::vector<std::wstring> result{};
		result.reserve(size);

		return result;
	}

	// �v�f��ǉ�
	template<std::size_t CharBufferSize>
	static void emplace_back(std::vector<std::wstring>& texture_path, std::size_t size, std::array<char_type, CharBufferSize> const& str)
	{
		texture_path.emplace_back(&str[0], size);
	}
};

template<>
struct mmdl::pmx_material_traits<std::pair<std::vector<material_data>, std::vector<material_data_2>>>
{
	using char_type = wchar_t;

	// �T�C�Y���w�肵�č\�z
	static std::pair<std::vector<material_data>, std::vector<material_data_2>> construct(std::size_t size)
	{
		std::vector<material_data> material{};
		std::vector<material_data_2> texture_material{};

		material.reserve(size);
		texture_material.reserve(size);

		return std::make_pair(std::move(material), std::move(texture_material));
	}

	// �v�f��ǉ�
	template<std::size_t CharBufferSize>
	static void emplace_back(auto& mat, pmx_material_buffer<char_type, CharBufferSize> const& buffer)
	{
		auto& [material, material_texture] = mat;

		auto sm = [&buffer]() {
			switch (buffer.sphere_mode) {
			case 1:
				return sphere_mode::sph;
			case 2:
				return sphere_mode::spa;
			case 3:
				return sphere_mode::subtexture;
			}

			return sphere_mode::none;
		}();

		material_data m{
			.diffuse = {buffer.diffuse[0],buffer.diffuse[1],buffer.diffuse[2],buffer.diffuse[3]},
			.specular = {buffer.specular[0],buffer.specular[1],buffer.specular[2]},
			.specularity = buffer.specularity,
			.ambient = {buffer.ambient[0],buffer.ambient[1],buffer.ambient[2]}
		};

		material_data_2 mt{
			.texture_index = buffer.texture_index,
			.sphere_mode = sm,
			.sphere_texture_index = buffer.sphere_texture_index,
			.toon_type = buffer.toon_flag == 0 ? toon_type::unshared : toon_type::shared,
			.toon_texture = buffer.toon_index,
			.vertex_number = static_cast<std::size_t>(buffer.vertex_num)
		};

		material.push_back(m);
		material_texture.push_back(mt);
	}
};


template<>
struct mmdl::pmx_bone_traits<std::vector<model_bone_data>>
{
	using char_type = wchar_t;

	// �T�C�Y���w�肵�č\�z
	static std::vector<model_bone_data> construct(std::size_t size)
	{
		std::vector<model_bone_data> result{};
		result.reserve(size);
		return result;
	}

	// �v�f��ǉ�
	template<std::size_t CharBufferSize, std::size_t IKLinkBufferSize>
	static void emplace_back(std::vector<model_bone_data>& bone, pmx_bone_buffer<char_type, CharBufferSize, IKLinkBufferSize> const& buffer)
	{
		model_bone_data b{};
		b.name = std::wstring(&buffer.name[0], buffer.name_size);
		b.position = { buffer.position[0],buffer.position[1] ,buffer.position[2] };
		b.parent_index = buffer.parent_bone_index;
		b.bone_flag_bits = { buffer.bone_flag };

		b.grant_index = buffer.assign_bone_index;
		b.grant_rate = buffer.assign_rate;

		b.ik_target_bone = buffer.ik_target_bone;
		b.ik_roop_number = buffer.ik_loop_num;
		b.ik_rook_angle = buffer.ik_angle_limit_par_process;

		b.ik_link.reserve(buffer.ik_link_size);
		for (std::size_t i = 0; i < buffer.ik_link_size; i++)
		{
			::ik_link il{};
			auto const& [bone, is_limit, bottom, top] = buffer.ik_link[i];
			
			il.bone = bone;
			if (is_limit)
			{
				il.min_max_angle_limit = std::make_pair(XMFLOAT3{ bottom[0],bottom[1] ,bottom[2] }, XMFLOAT3{ top[0],top[1] ,top[2] });
			}

			b.ik_link.push_back(il);
		}

		bone.push_back(b);
	}
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
