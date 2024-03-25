#pragma once
#include<utility>
#include<vector>
#include<limits>
#include<DirectXMath.h>
#include<unordered_map>
#include<string>
#include<algorithm>
#include"../external/mmd-loader/mmdl/vpd/loader.hpp"
#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"struct.hpp"
#include<tuple>


using namespace DirectX;

//
// ���\�[�X����
//

// �P�F��4x4�̑傫���̃e�N�X�`���̃��\�[�X��Ԃ�
template<typename T, typename U>
dx12w::resource_and_state get_fill_4x4_texture_resource(T& device, U& command_manager, DXGI_FORMAT format, std::uint8_t value);

// pmx�̃e�N�X�`���̃��\�[�X�̎擾
template<typename T, typename U, typename S>
std::vector<dx12w::resource_and_state> get_pmx_texture_resource(T& device, U& command_manager, S const& pmx_texture_path, DXGI_FORMAT format, std::wstring const& directory_path);

// pmx�̃}�e���A���̃��\�[�X�̎擾
template<typename T, typename U>
std::vector<dx12w::resource_and_state> get_pmx_material_resource(T& device, U const& pmx_material);


//
// pmx
//

// �{�[���̖��O����{�[���̃C���f�b�N�X��Ԃ�unordered_map�̐���
template<typename T>
std::unordered_map<std::wstring_view, std::size_t> get_bone_name_to_bone_index(T const& pmx_bone);

// ���[�t��pmx�̃C���f�b�N�X���烂�[�t���i�[����Ă���z��̃C���f�b�N�X��Ԃ�unordered_map�̐���
template<typename T>
std::unordered_map<std::size_t, std::size_t> get_morph_index_to_morph_vertex_index(T const& pmx_morph);

// �q�{�[���ւ̃C���f�b�N�X�̃��X�g���쐬
template<typename T>
std::vector<std::vector<std::size_t>> get_to_children_bone_index(T const& pmx_bone);


//
// vpd�Avmd
//

// vpd�̃f�[�^���{�[���ɔ��f������
template<typename T, typename U, typename S>
void set_bone_data_from_vpd(T& bone_data, U const& vpd_data, S const& bone_name_to_bone_index);

// vmd�̃f�[�^���g���₷���悤�ɕϊ�
template<typename T>
std::unordered_map<std::wstring, std::vector<bone_motion_data>> get_bone_name_to_bone_motion_data(T&& vmd_data);

// vmd�̃��[�t�̃f�[�^���g���₷���悤�ɕϊ�
template<typename T>
std::unordered_map<std::wstring, std::vector<morph_motion_data>> get_morph_name_to_morph_motion_data(T&& vmd_data);

// vmd�̃f�[�^���{�[���ɔ��f������
template<typename T, typename U, typename S, typename R>
void set_bone_data_from_vmd(T& bone_data, U const& bone_name_to_bone_motion_data, S const& pmx_bone, R const& bone_name_to_bone_index, std::size_t frame_num);

// ���̃t���[���̃��[�t�̃E�F�C�g����`�⊮���Ԃ�
template<typename T>
float get_morph_motion_weight(T const& morph_data, std::wstring const& morph_name, std::size_t frame_num);

//
// bone_data
//

// �{�[���f�[�^�̏�����
template<typename T>
void initialize_bone_data(T& bone_data);

// �ċA�I�ɑ������e�{�[���̏���ێ�������
template<typename T, typename U, typename S>
void set_to_world_matrix(T& bone_data, U const& to_children_index, std::size_t current_index, XMMATRIX const& parent_matrix, S const& pmx_bone);

// ccdik����
void solve_CCDIK(std::vector<bone_data>& bone, std::size_t root_index, auto const& pmx_bone, XMFLOAT3& target_position,
	std::vector<std::vector<std::size_t>> const& to_children_bone_index, int debug_ik_rotation_num, int* debug_ik_rotation_counter, bool debug_check_ideal_rotation);

// �ċA�I�Ƀ{�[�������ǂ��Ă���ik�̏������s��
template<typename T, typename U, typename S>
void recursive_aplly_ik(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone, int debug_ik_rotation_num = -1, int* debug_ik_rotation_counter = nullptr, bool debug_check_ideal_rotation = false);

// ��]�t�^�A�ړ��t�^�̏���
template<typename T, typename U, typename S>
void solve_grant(T& bone_data, U const& pmx_data, std::size_t index, S const& to_children_index);

// ��]�t�^�A�ړ��t�^�̏������ċA�I�ɂ���Ă�
template<typename T, typename U, typename S>
void recursive_aplly_grant(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone);

// �{�[���̃f�[�^����{�[���s��̐���
template<typename T, typename U, typename S>
void bone_data_to_bone_matrix(T& bone_data, U& bone_matrix, S const& pmx_bone);


//
// ���̑�
//

// �x�W�F�Ȑ��̌v�Z
inline float calc_bezier_curve(float x, float p1_x, float p1_y, float p2_x, float p2_y);

// �N�H�[�^�j�I���̃N�����v
inline bool clamp_quaternion(XMVECTOR& q, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);

// ansi����UTF16�ɕϊ�
inline std::wstring ansi_to_utf16(std::string_view const& src);


//
// �ȉ��A����
//

template<typename T, typename U>
dx12w::resource_and_state get_fill_4x4_texture_resource(T& device, U& command_manager, DXGI_FORMAT format, std::uint8_t value)
{
	// �傫����4x4
	auto result = dx12w::create_commited_texture_resource(device.get(),
		format, 4, 4, 2, 1, 1, D3D12_RESOURCE_FLAG_NONE);

	auto const dst_desc = result.first->GetDesc();

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT src_footprint;
	UINT64 src_total_bytes;
	device->GetCopyableFootprints(&dst_desc, 0, 1, 0, &src_footprint, nullptr, nullptr, &src_total_bytes);

	auto src_texture_resorce = dx12w::create_commited_upload_buffer_resource(device.get(), src_total_bytes);
	std::uint8_t* tmp = nullptr;
	src_texture_resorce.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));

	// �w��̒l���Z�b�g����
	std::fill(tmp, tmp + src_total_bytes, value);

	D3D12_TEXTURE_COPY_LOCATION src_texture_copy_location{
			.pResource = src_texture_resorce.first.get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint = src_footprint,
	};

	D3D12_TEXTURE_COPY_LOCATION dst_texture_copy_location{
		.pResource = result.first.get(),
		.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
	};

	command_manager.reset_list(0);
	dx12w::resource_barrior(command_manager.get_list(), result, D3D12_RESOURCE_STATE_COPY_DEST);
	command_manager.get_list()->CopyTextureRegion(&dst_texture_copy_location, 0, 0, 0, &src_texture_copy_location, nullptr);
	dx12w::resource_barrior(command_manager.get_list(), result, D3D12_RESOURCE_STATE_COMMON);
	command_manager.get_list()->Close();
	command_manager.excute();
	command_manager.signal();
	command_manager.wait(0);

	return std::move(result);
}

template<typename T, typename U, typename S>
std::vector<dx12w::resource_and_state> get_pmx_texture_resource(T& device, U& command_manager, S const& pmx_texture_path, DXGI_FORMAT format, std::wstring const& directory_path)
{
	std::vector<dx12w::resource_and_state> pmx_texture_resrouce{};

	for (auto& path : pmx_texture_path)
	{
		char buff[256];
		stbi_convert_wchar_to_utf8(buff, 256, (std::wstring{ directory_path } + path).data());
		int x, y, n;
		std::uint8_t* data = stbi_load(buff, &x, &y, &n, 0);

		auto dst_texture_resource = dx12w::create_commited_texture_resource(device.get(),
			format, x, y, 2, 1, 1, D3D12_RESOURCE_FLAG_NONE);

		auto const dst_desc = dst_texture_resource.first->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT src_footprint;
		UINT64 src_total_bytes;
		device->GetCopyableFootprints(&dst_desc, 0, 1, 0, &src_footprint, nullptr, nullptr, &src_total_bytes);

		auto src_texture_resorce = dx12w::create_commited_upload_buffer_resource(device.get(), src_total_bytes);
		std::uint8_t* tmp = nullptr;
		src_texture_resorce.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));

		for (std::size_t y_i = 0; y_i < y; y_i++)
		{
			for (std::size_t x_i = 0; x_i < x; x_i++)
			{
				for (std::size_t n_i = 0; n_i < n; n_i++)
				{
					tmp[y_i * src_footprint.Footprint.RowPitch + x_i * 4 + n_i] = data[(y_i * x + x_i) * n + n_i];
				}

				if (n == 3)
				{
					tmp[y_i * src_footprint.Footprint.RowPitch + x_i * 4 + 3] = 255;
				}
			}
		}

		D3D12_TEXTURE_COPY_LOCATION src_texture_copy_location{
			.pResource = src_texture_resorce.first.get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint = src_footprint,
		};

		D3D12_TEXTURE_COPY_LOCATION dst_texture_copy_location{
			.pResource = dst_texture_resource.first.get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		};

		// ����excute�Ăяo���̂͂ǂ����Ǝv��
		// ���ǁA�y�Ȃ̂łƂ肠����
		command_manager.reset_list(0);
		dx12w::resource_barrior(command_manager.get_list(), dst_texture_resource, D3D12_RESOURCE_STATE_COPY_DEST);
		command_manager.get_list()->CopyTextureRegion(&dst_texture_copy_location, 0, 0, 0, &src_texture_copy_location, nullptr);
		dx12w::resource_barrior(command_manager.get_list(), dst_texture_resource, D3D12_RESOURCE_STATE_COMMON);
		command_manager.get_list()->Close();
		command_manager.excute();
		command_manager.signal();
		command_manager.wait(0);

		// TODO: �T�C�Y�������Ă邩��reserve����悤�ɕύX����
		pmx_texture_resrouce.emplace_back(std::move(dst_texture_resource));
	}

	return pmx_texture_resrouce;
}

template<typename T, typename U>
std::vector<dx12w::resource_and_state> get_pmx_material_resource(T& device, U const& pmx_material)
{
	std::vector<dx12w::resource_and_state> material_resource{};

	for (auto& material : pmx_material)
	{
		auto tmp_material_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(material_data), 256));

		material_data* tmp = nullptr;
		tmp_material_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		tmp->diffuse = material.diffuse;
		tmp->specular = material.specular;
		tmp->specularity = material.specularity;
		tmp->ambient = material.ambient;
		tmp_material_resource.first->Unmap(0, nullptr);

		material_resource.emplace_back(std::move(tmp_material_resource));
	}

	return material_resource;
}

template<typename T>
std::unordered_map<std::wstring_view, std::size_t> get_bone_name_to_bone_index(T const& pmx_bone)
{
	std::unordered_map<std::wstring_view, std::size_t> result{};

	result.reserve(pmx_bone.size());

	for (std::size_t i = 0; i < pmx_bone.size(); i++)
	{
		result.emplace(pmx_bone[i].name, i);
	}

	return result;
}

template<typename T>
std::unordered_map<std::size_t,std::size_t> get_morph_index_to_morph_vertex_index(T const& pmx_morph)
{
	std::unordered_map<std::size_t, std::size_t> result{};

	result.reserve(pmx_morph.size());

	for (std::size_t i = 0; i < pmx_morph.size(); i++)
	{
		result.emplace(pmx_morph[i].morph_index, i);
	}

	return result;
}

template<typename T>
std::vector<std::vector<std::size_t>> get_to_children_bone_index(T const& pmx_bone)
{
	std::vector<std::vector<std::size_t>> result(pmx_bone.size());

	for (std::size_t i = 0; i < pmx_bone.size(); i++)
	{
		if (0 <= pmx_bone[i].parent_index && pmx_bone[i].parent_index < pmx_bone.size())
		{
			result[pmx_bone[i].parent_index].push_back(i);
		}
	}

	return result;
}

template<typename T, typename U, typename S>
void set_bone_data_from_vpd(T& bone_data, U const& vpd_data, S const& bone_name_to_bone_index)
{
	for (auto const& vpd : vpd_data)
	{
		// �Ώۂ̃{�[�������݂��Ȃ��ꍇ�͔�΂�
		if (!bone_name_to_bone_index.contains(vpd.name)) {
			continue;
		}

		auto const index = bone_name_to_bone_index.at(vpd.name);

		bone_data[index].rotation = XMLoadFloat4(&vpd.quaternion);
		bone_data[index].transform = XMLoadFloat3(&vpd.transform);
	}
}

template<typename T>
std::unordered_map<std::wstring, std::vector<bone_motion_data>> get_bone_name_to_bone_motion_data(T&& vmd_data)
{
	std::unordered_map<std::wstring, std::vector<bone_motion_data>> result{};

	for (auto&& vmd : std::forward<T>(vmd_data))
	{
		result[vmd.name].emplace_back(static_cast<int>(vmd.frame_num), vmd.transform, vmd.quaternion,
			std::array<char, 2>{vmd.complement_parameter[3], vmd.complement_parameter[7] }, std::array<char, 2>{vmd.complement_parameter[11], vmd.complement_parameter[15]},
			std::array<char, 2>{ vmd.complement_parameter[0], vmd.complement_parameter[4] }, std::array<char, 2>{ vmd.complement_parameter[8], vmd.complement_parameter[12] },
			std::array<char, 2>{ vmd.complement_parameter[1], vmd.complement_parameter[5] }, std::array<char, 2>{ vmd.complement_parameter[9], vmd.complement_parameter[13] },
			std::array<char, 2>{ vmd.complement_parameter[2], vmd.complement_parameter[6] }, std::array<char, 2> { vmd.complement_parameter[10], vmd.complement_parameter[14] }
		);
	}

	// ��Ō����ł���悤�ɂ��ꂼ��̃��[�V�������t���[���ԍ����Ƀ\�[�g
	for (auto& motion : result)
	{
		std::sort(motion.second.begin(), motion.second.end(), [](auto const& a, auto const& b) {return a.frame_num < b.frame_num; });
	}

	return result;
}

template<typename T>
std::unordered_map<std::wstring, std::vector<morph_motion_data>> get_morph_name_to_morph_motion_data(T&& vmd_data)
{
	std::unordered_map<std::wstring, std::vector<morph_motion_data>> result{};

	for (auto&& vmd : std::forward<T>(vmd_data))
	{
		result[vmd.name].emplace_back(static_cast<int>(vmd.frame_num), vmd.weight);
	}

	// ��Ō������₷���悤�Ƀ\�[�g
	for (auto& motion : result)
	{
		std::sort(motion.second.begin(), motion.second.end(), [](auto const& a, auto const& b) {return a.frame_num < b.frame_num; });
	}

	return result;
}

template<typename T, typename U, typename S, typename R>
void set_bone_data_from_vmd(T& bone_data, U const& bone_name_to_bone_motion_data, S const& pmx_bone, R const& bone_name_to_bone_index, std::size_t frame_num)
{
	for (auto const& motion : bone_name_to_bone_motion_data)
	{
		auto const bone_name_and_index = bone_name_to_bone_index.find(motion.first);

		// �Ώۂ̃{�[�������݂��Ȃ��ꍇ�͔�΂�
		if (bone_name_and_index == bone_name_to_bone_index.end()) {
			continue;
		}

		auto const& bone_position = pmx_bone[bone_name_and_index->second].position;

		auto const& motion_data = motion.second;

		// �z��̖������猟�����t���[�������傫���A���t���[�����Ɉ�ԋ߂����[�V�����f�[�^����������
		auto const curr_motion_riter = std::find_if(motion_data.rbegin(), motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

		// �Ώۂ̃��[�V�����f�[�^�����݂��Ȃ��ꍇ�͔�΂�
		if (curr_motion_riter == motion_data.crend()) {
			continue;
		}

		// ���O�̃{�[���̃��[�V�����f�[�^���Q�Ƃł���
		auto const prev_motion_iter = curr_motion_riter.base();

		// ��]�̌v�Z
		auto const [rotation, transform] = [curr_motion_riter, prev_motion_iter, &motion_data, frame_num]() {
			if (prev_motion_iter != motion_data.end()) {
				auto const t = static_cast<float>(frame_num - curr_motion_riter->frame_num) / static_cast<float>(prev_motion_iter->frame_num - curr_motion_riter->frame_num);

				// �x�W�F�Ȑ��𗘗p�������ꂼ��̃p�����[�^���v�Z
				auto const x_t = calc_bezier_curve(t, prev_motion_iter->x_a[0] / 127.f, prev_motion_iter->x_a[1] / 127.f, prev_motion_iter->x_b[0] / 127.f, prev_motion_iter->x_b[1] / 127.f);
				auto const y_t = calc_bezier_curve(t, prev_motion_iter->y_a[0] / 127.f, prev_motion_iter->y_a[1] / 127.f, prev_motion_iter->y_b[0] / 127.f, prev_motion_iter->y_b[1] / 127.f);
				auto const z_t = calc_bezier_curve(t, prev_motion_iter->z_a[0] / 127.f, prev_motion_iter->z_a[1] / 127.f, prev_motion_iter->z_b[0] / 127.f, prev_motion_iter->z_b[1] / 127.f);
				auto const r_t = calc_bezier_curve(t, prev_motion_iter->r_a[0] / 127.f, prev_motion_iter->r_a[1] / 127.f, prev_motion_iter->r_b[0] / 127.f, prev_motion_iter->r_b[1] / 127.f);

				// ���`���
				auto const x = std::lerp(curr_motion_riter->transform.x, prev_motion_iter->transform.x, x_t);
				auto const y = std::lerp(curr_motion_riter->transform.y, prev_motion_iter->transform.y, y_t);
				auto const z = std::lerp(curr_motion_riter->transform.z, prev_motion_iter->transform.z, z_t);
				auto const transform = XMVECTOR{ x,y,z };

				// ���ʕ��
				auto const rotation = XMQuaternionSlerp(XMLoadFloat4(&curr_motion_riter->quaternion), XMLoadFloat4(&prev_motion_iter->quaternion), r_t);

				return std::make_pair(rotation, transform);
			}
			else {
				return std::make_pair(XMLoadFloat4(&curr_motion_riter->quaternion), XMVECTOR{ curr_motion_riter->transform.x, curr_motion_riter->transform.y, curr_motion_riter->transform.z });
			}
		}();

		// �{�[���f�[�^�ɔ��f
		bone_data[bone_name_and_index->second].rotation = rotation;
		bone_data[bone_name_and_index->second].transform = transform;
	}
}

template<typename T>
float get_morph_motion_weight(T const& morph_data, std::wstring const& morph_name, std::size_t frame_num)
{
	auto const iter = morph_data.find(morph_name);

	// �Ώۂ̃��[�t�̃A�j���[�V�����̃f�[�^���Ȃ��ꍇ
	if (iter == morph_data.end())
		return 0.f;

	auto const& motion_data = iter->second;

	// �z��̖������猟�����t���[�������傫���A���t���[�����Ɉ�ԋ߂����[�V�����f�[�^����������
	auto const curr_motion_riter = std::find_if(motion_data.rbegin(), motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

	// �Ώۂ̃��[�V�����f�[�^�����݂��Ȃ��ꍇ
	if (curr_motion_riter == motion_data.crend()) 
		return 0.f;

	// ���O�̃{�[���̃��[�V�����f�[�^���Q��
	auto const prev_motion_iter = curr_motion_riter.base();

	// ��O�̃��[�V�����f�[�^�����݂��Ȃ��ꍇ
	if (prev_motion_iter == motion_data.end())
		return 0.f;

	auto const t = static_cast<float>(frame_num - curr_motion_riter->frame_num) / static_cast<float>(prev_motion_iter->frame_num - curr_motion_riter->frame_num);

	// ���`�ۊǂ��ďd�݂�Ԃ�
	return std::lerp(curr_motion_riter->weight, prev_motion_iter->weight, t);

}

template<typename T>
void initialize_bone_data(T& bone_data)
{
	for (std::size_t i = 0; i < bone_data.size(); i++)
	{
		bone_data[i].rotation = XMQuaternionIdentity();
		bone_data[i].transform = {};
		bone_data[i].to_world = XMMatrixIdentity();
	}
}

template<typename T, typename U, typename S>
void set_to_world_matrix(T& bone_data, U const& to_children_index, std::size_t current_index, XMMATRIX const& parent_matrix, S const& pmx_bone)
{
	bone_data[current_index].to_world = parent_matrix;

	for (auto const children_index : to_children_index[current_index])
	{
		auto rotaion_origin = XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[current_index].position)) *
			XMMatrixRotationQuaternion(bone_data[current_index].rotation) *
			XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[current_index].position));

		auto current_matrix = rotaion_origin * XMMatrixTranslationFromVector(bone_data[current_index].transform) * parent_matrix;

		set_to_world_matrix(bone_data, to_children_index, children_index, current_matrix, pmx_bone);
	}
}

void solve_CCDIK(std::vector<bone_data>& bone, std::size_t root_index, auto const& pmx_bone, XMFLOAT3& target_position,
	std::vector<std::vector<std::size_t>> const& to_children_bone_index, int debug_ik_rotation_num, int* debug_ik_rotation_counter, bool debug_check_ideal_rotation)
{
	auto target_index = pmx_bone[root_index].ik_target_bone;

	// ���[�v����IK���������Ă���
	for (std::size_t ik_roop_i = 0; ik_roop_i < static_cast<std::size_t>(pmx_bone[root_index].ik_roop_number); ik_roop_i++)
	{

		// ���ꂼ��̃{�[���𓮂����Ă���
		for (std::size_t ik_link_i = 0; ik_link_i < pmx_bone[root_index].ik_link.size(); ik_link_i++)
		{

			if (debug_ik_rotation_counter != nullptr && debug_ik_rotation_num >= 0 && *debug_ik_rotation_counter >= debug_ik_rotation_num) {
				return;
			}

			// ��]�̑Ώۂł���ik_link�̃{�[��
			auto const& ik_link = pmx_bone[root_index].ik_link[ik_link_i];

			// �Ώۂ�ik_link�̃{�[���̍��W�n���烏�[���h���W�ւ̕ϊ�
			auto const to_world =
				XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[ik_link.bone].position)) *
				XMMatrixRotationQuaternion(bone[ik_link.bone].rotation) *
				XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[ik_link.bone].position)) *
				XMMatrixTranslationFromVector(bone[ik_link.bone].transform) *
				bone[ik_link.bone].to_world;

			// ���[���h���W�n����Ώۂ�ik_link�̃{�[���̍��W�n�ւ̕ϊ�
			// WARNING: �t�s��v�Z�̃R�X�g��?
			auto const to_local = XMMatrixInverse(nullptr, to_world);

			// ���݂̃^�[�Q�b�g�̃{�[���̃��[���h���W�ł̈ʒu
			auto const world_current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position),
				XMMatrixTranslationFromVector(bone[target_index].transform) *
				bone[target_index].to_world);

			// ���݂̃^�[�Q�b�g�̃{�[���̑Ώۂ�ik_link�̃{�[���̍��W�n�̈ʒu
			auto const local_current_target_position = XMVector3Transform(world_current_target_position, to_local);

			// �Ώۂ̃{�[���̃��[���h���W�ł̈ʒu
			auto const world_ik_link_bone_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[ik_link.bone].position), to_world);

			// �Ώۂ̃{�[���̍��W�n�ł̑Ώۂ̃{�[���̈ʒu
			// �܂�A���̂܂܂̍��W
			auto const local_ik_link_bone_position = XMLoadFloat3(&pmx_bone[ik_link.bone].position);

			// �Ώۂ̃{�[���̍��W�n�ł̃^�[�Q�b�g�̃{�[���̗��z�I�Ȉʒu
			// �ilocal_current_target_position �� local_target_position �֋߂Â���̂��ړI�j
			auto const local_target_position = XMVector3Transform(XMLoadFloat3(&target_position), to_local);

			// �Ώۂ̃{�[�����猻�݂̃^�[�Q�b�g�ւ̃x�N�g��
			auto const to_current_target = XMVector3Normalize(XMVectorSubtract(local_current_target_position, local_ik_link_bone_position));

			// �Ώۂ̃{�[������^�[�Q�b�g�ւ̃x�N�g��
			auto const to_target = XMVector3Normalize(XMVectorSubtract(local_target_position, local_ik_link_bone_position));


			// �قړ����x�N�g���ɂȂ��Ă��܂����ꍇ�͊O�ς��v�Z�ł��Ȃ����ߔ�΂�
			if (XMVector3Length(XMVectorSubtract(to_current_target, to_target)).m128_f32[0] <= std::numeric_limits<float>::epsilon()) {
				continue;
			}

			// �O�ς���ъp�x�̌v�Z
			auto const cross = XMVector3Normalize(XMVector3Cross(to_current_target, to_target));
			auto const angle = XMVector3AngleBetweenVectors(to_current_target, to_target).m128_f32[0];

			// �p�x�������l�����Ȃ����z�I�ȉ�]
			auto const ideal_rotation = XMQuaternionMultiply(XMQuaternionRotationMatrix(XMMatrixRotationAxis(cross, angle)), bone[ik_link.bone].rotation);

			// �f�o�b�N�p
			// �p�����𖳎������ꍇ�̉�]��\�����邽�߂̃t���O
			bool const use_ideal_rotation_for_debug = debug_check_ideal_rotation && debug_ik_rotation_counter != nullptr && *debug_ik_rotation_counter >= debug_ik_rotation_num - 1;

			// �p�x�̐������l���������ۂ̉�]��\���s��Ɖ�]�̍s�񂪏C�����ꂽ���ǂ���
			auto const actual_rotation = [&cross, &angle, &ik_link, use_ideal_rotation_for_debug](auto const& ideal_rotation) {

				// �f�o�b�O�ړI�ŗ��z��]���m�F����Ƃ�
				if (use_ideal_rotation_for_debug) {
					return ideal_rotation;
				}

				// �������Ȃ��ꍇ���̂܂�
				if (!ik_link.min_max_angle_limit)
				{
					return ideal_rotation;
				}
				// ����������ꍇ�͒�������
				else
				{
					auto const& [angle_limit_min, angle_limit_max] = ik_link.min_max_angle_limit.value();
					// �R�s�[����
					auto result = ideal_rotation;

					// �N�����v
					clamp_quaternion(result, angle_limit_min.x, angle_limit_max.x, angle_limit_min.y, angle_limit_max.y, angle_limit_min.z, angle_limit_max.z);

					return result;
				}
			}(ideal_rotation);

			// ��]�𔽉f
			bone[ik_link.bone].rotation = actual_rotation;

			// �Ώۂ�ik_link�̃{�[����薖�[�̃{�[���ɉ�]��K�p����
			set_to_world_matrix(bone, to_children_bone_index, ik_link.bone, bone[ik_link.bone].to_world, pmx_bone);

			// �f�o�b�O�p
			// ik�̏����ɂ���ĉ�]�����񐔂��L�^����
			if (debug_ik_rotation_counter) {
				(*debug_ik_rotation_counter)++;
			}

			// �^�[�Q�b�g�̃{�[���̈ʒu�̍X�V
			auto const updated_world_current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position),
				XMMatrixTranslationFromVector(bone[target_index].transform) *
				bone[target_index].to_world);

			// �\���߂��Ȃ����ꍇ���[�v�𔲂���
			if (XMVector3Length(XMVectorSubtract(updated_world_current_target_position, XMLoadFloat3(&target_position))).m128_f32[0] <= std::numeric_limits<float>::epsilon()) {
				break;
			}
		}
	}
}

template<typename T, typename U, typename S>
void recursive_aplly_ik(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone, int debug_ik_rotation_num, int* debug_ik_rotation_counter, bool debug_check_ideal_rotation)
{
	// ���݂̑Ώۂ̃{�[����ik�̏ꍇ
	if (pmx_bone[current_index].bone_flag_bits[static_cast<std::size_t>(bone_flag::ik)])
	{
		auto const target_bone_index = pmx_bone[current_index].ik_target_bone;
		auto const target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_bone_index].position),
			XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[current_index].position)) *
			XMMatrixRotationQuaternion(bone[current_index].rotation) *
			XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[current_index].position)) *
			XMMatrixTranslationFromVector(bone[current_index].transform) *
			bone[current_index].to_world);

		// TODO:
		XMFLOAT3 float3;
		XMStoreFloat3(&float3, target_position);

		solve_CCDIK(bone, current_index, pmx_bone, float3, to_children_bone_index, debug_ik_rotation_num, debug_ik_rotation_counter, debug_check_ideal_rotation);

	}

	// �ċA�I�Ɏq�{�[�������ǂ��Ă���
	for (auto children_index : to_children_bone_index[current_index])
	{
		recursive_aplly_ik(bone, children_index, to_children_bone_index, pmx_bone, debug_ik_rotation_num, debug_ik_rotation_counter, debug_check_ideal_rotation);
	}
}


template<typename T, typename U, typename S>
void solve_grant(T& bone_data, U const& pmx_data, std::size_t index, S const& to_children_index)
{
	// �Ώۂ̃{�[������]�t�^�̏ꍇ
	if (pmx_data[index].bone_flag_bits[static_cast<std::size_t>(bone_flag::rotation_grant)])
	{
		// ���̕����̉�]
		if (pmx_data[index].grant_rate >= 0.f)
		{
			// �ǉ����̉�]���v�Z
			auto const additional_rotation = XMQuaternionSlerp(XMQuaternionIdentity(), bone_data[pmx_data[index].grant_index].rotation, pmx_data[index].grant_rate);

			// ���f
			bone_data[index].rotation = XMQuaternionMultiply(bone_data[index].rotation, additional_rotation);
			set_to_world_matrix(bone_data, to_children_index, index, bone_data[index].to_world, pmx_data);
		}
		// ���̕����̉�]
		else
		{
			// �����𐳂ɂ���
			auto const modified_rate = -pmx_data[index].grant_rate;
			// ��]�������C������
			auto const rotation_inv = XMQuaternionInverse(bone_data[pmx_data[index].grant_index].rotation);

			// �ǉ����̉�]���v�Z
			auto const additional_rotation = XMQuaternionSlerp(XMQuaternionIdentity(), rotation_inv, modified_rate);

			// ���f
			bone_data[index].rotation = XMQuaternionMultiply(bone_data[index].rotation, additional_rotation);
			set_to_world_matrix(bone_data, to_children_index, index, bone_data[index].to_world, pmx_data);
		}
	}

	// �Ώۂ̃{�[�����ړ��t�^�̏ꍇ
	if (pmx_data[index].bone_flag_bits[static_cast<std::size_t>(bone_flag::move_grant)])
	{
		// �ǉ����̈ړ�
		auto const additional_transform = XMVectorScale(bone_data[pmx_data[index].grant_index].transform, pmx_data[index].grant_rate);

		// ���f
		bone_data[index].transform += additional_transform;
		set_to_world_matrix(bone_data, to_children_index, index, bone_data[index].to_world, pmx_data);
	}
}

template<typename T, typename U, typename S>
void recursive_aplly_grant(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone)
{
	solve_grant(bone, pmx_bone, current_index, to_children_bone_index);

	// �ċA�I�Ɏq�{�[�������ǂ��Ă���
	for (auto children_index : to_children_bone_index[current_index])
	{
		recursive_aplly_grant(bone, children_index, to_children_bone_index, pmx_bone);
	}
}


template<typename T, typename U, typename S>
void bone_data_to_bone_matrix(T& bone_data, U& bone_matrix, S const& pmx_bone)
{
	for (std::size_t i = 0; i < pmx_bone.size(); i++)
	{
		bone_matrix[i] =
			XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[i].position)) *
			XMMatrixRotationQuaternion(bone_data[i].rotation) *
			XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[i].position)) *
			XMMatrixTranslationFromVector(bone_data[i].transform) *
			bone_data[i].to_world;
	}
}

inline float calc_bezier_curve(float x, float p1_x, float p1_y, float p2_x, float p2_y)
{
	// �j���[�g���@��K�p����x��t�ɂ��Ă̎�
	auto f = [x, p1_x, p2_x](float t) {
		return (1.f + 3.f * p1_x - 3.f * p2_x) * t * t * t
			+ (3.f * p2_x - 6.f * p1_x) * t * t + 3.f * p1_x * t - x;
	};

	// f�̓��֐�
	auto df = [x, p1_x, p2_x](float t) {
		return 3.f * (1.f + 3.f * p1_x - 3.f * p2_x) * t * t + 2.f * (3.f * p2_x - 6.f * p1_x) * t + 3.f * p1_x;
	};

	// �����l
	float t = 0.5;

	// ��������ł���
	for (std::size_t i = 0; i < 5; i++)
	{
		t -= f(t) / df(t);
	}

	// y�̒l
	return 3.f * (1.f - t) * (1.f - t) * t * p1_y + 3.f * (1.f - t) * t * t * p2_y + t * t * t;
}

inline bool clamp_quaternion(XMVECTOR& q, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max)
{
	// ���ꂼ��̎�����̉�]���擾�ł���悤�ɃN�H�[�^�j�I�����C��
	q.m128_f32[0] /= q.m128_f32[3];
	q.m128_f32[1] /= q.m128_f32[3];
	q.m128_f32[2] /= q.m128_f32[3];
	q.m128_f32[3] = 1.f;

	// ���ۂɒl���ύX���ꂽ���ǂ���
	auto is_clamped = false;

	// x���̉�]�ɂ��Đ���
	
	// x���̉�]��\���N�I�[�^�j�I���ɂ���
	//  [1*sin(rot/2), 0*sin(rot/2), 0*sin(rot/2), cos(rot/2)]
	// =[sin(rot/2), 0, 0, cos(rot/2)]
	// w������1�ɂȂ�悤�ɐ��K�����s��
	// =[sin(rot/2)/cos(rot/2), 0, 0, 1]
	// =[tan(rot/2), 0, 0, 1]
	// min_rot��max_rot��tan(rot/2)�ŕϊ����ăN�����v

	// tan�̌v�Z���ł���悤�ɕ␳
	x_min = x_min < -3.14 ? -3.14 : x_min;
	x_max = x_max > 3.14 ? 3.14 : x_max;

	auto const x_min_rot = std::tan(x_min * 0.5f);
	auto const x_max_rot = std::tan(x_max * 0.5f);
	auto const x_updated = std::clamp(q.m128_f32[0], x_min_rot, x_max_rot);
	if (q.m128_f32[0] != x_updated) {
		q.m128_f32[0] = x_updated;
		is_clamped = true;
	}

	// y���̉�]�ɂ��Đ���
	y_min = y_min < -3.14 ? -3.14 : y_min;
	y_max = y_max > 3.14 ? 3.14 : y_max;
	auto const y_min_rot = std::sin(y_min * 0.5f);
	auto const y_max_rot = std::sin(y_max * 0.5f);
	auto const y_updated = std::clamp(q.m128_f32[1], y_min_rot, y_max_rot);
	if (q.m128_f32[1] != y_updated) {
		q.m128_f32[1] = y_updated;
		is_clamped = true;
	}

	// z���̉�]�ɂ���
	z_min = z_min < -3.14 ? -3.14 : z_min;
	z_max = z_max > 3.14 ? 3.14 : z_max;
	auto const z_min_rot = std::sin(z_min * 0.5f);
	auto const z_max_rot = std::sin(z_max * 0.5f);
	auto const z_updated = std::clamp(q.m128_f32[2], z_min_rot, z_max_rot);
	if (q.m128_f32[2] != z_updated) {
		q.m128_f32[2] = z_updated;
		is_clamped = true;
	}

	q = XMQuaternionNormalize(q);

	return is_clamped;
}

inline std::wstring ansi_to_utf16(std::string_view const& src)
{
	// �ԊҌ�̑傫���̎擾
	auto dst_size = MultiByteToWideChar(CP_ACP, 0, src.data(), -1, nullptr, 0);

	// �I�_������ǉ����Ȃ�
	dst_size--;

	// �T�C�Y�̕ύX
	std::wstring result;
	result.resize(dst_size);

	// �ϊ�
	MultiByteToWideChar(CP_ACP, 0, src.data(), -1, result.data(), dst_size);

	return result;
}


