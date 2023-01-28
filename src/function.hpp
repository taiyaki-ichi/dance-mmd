#pragma once
#include<utility>
#include<vector>
#include<limits>
#include<DirectXMath.h>
#include<unordered_map>
#include<string>
#include<algorithm>
#include"../external/mmd-loader/mmdl/vpd_loader.hpp"
#include"struct.hpp"

using namespace DirectX;

// �q�{�[���ւ̃C���f�b�N�X�̃��X�g���쐬
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

template<typename T, typename U, typename S, typename R>
void set_bone_matrix_from_vpd(T& bone_matrix_container, U const& vpd_data, S const pmx_bone, R& bone_name_to_bone_index)
{
	for (auto const& vpd : vpd_data)
	{
		auto index = bone_name_to_bone_index[vpd.name];

		// ��]�̓K�p
		XMVECTOR quaternion_vector = XMLoadFloat4(&vpd.quaternion);
		bone_matrix_container[index] =
			XMMatrixTranslation(-pmx_bone[index].position.x, -pmx_bone[index].position.y, -pmx_bone[index].position.z) *
			XMMatrixRotationQuaternion(quaternion_vector) *
			XMMatrixTranslation(pmx_bone[index].position.x, pmx_bone[index].position.y, pmx_bone[index].position.z);

		// ����IK�p�̃p�����[�^���ۂ��H�Ԉ���Ă��邩��
		// �ړ��̓K�p
		bone_matrix_container[index] *= XMMatrixTranslation(vpd.transform.x, vpd.transform.y, vpd.transform.z);
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

template<typename T, typename U, typename S, typename R>
void set_bone_matrix_from_vmd(T& bone_matrix_container, U const& bone_name_to_bone_motion_data, S const pmx_bone, R& bone_name_to_bone_index, std::size_t frame_num)
{
	for (auto const& motion : bone_name_to_bone_motion_data)
	{
		auto index = bone_name_to_bone_index[motion.first];

		auto const& motion_data = motion.second;

		auto rit = std::find_if(motion_data.rbegin(), motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

		if (rit == motion_data.rend())
		{
			continue;
		}

		// ���O�̃{�[���̃��[�V�����f�[�^���Q�Ƃł���
		auto it = rit.base();

		auto quaternion = [rit, it, &motion_data, frame_num]() {
			XMVECTOR rit_quaternion = XMLoadFloat4(&rit->quaternion);
			if (it != motion_data.end()) {
				auto t = static_cast<float>(frame_num - rit->frame_num) / static_cast<float>(it->frame_num - rit->frame_num);
				auto y = calc_bezier_curve(t, it->p1_x / 127.f, it->p1_y / 127.f, it->p2_x / 127.f, it->p2_y / 127.f);
				XMVECTOR it_quaternion = XMLoadFloat4(&it->quaternion);
				return XMMatrixRotationQuaternion(XMQuaternionSlerp(rit_quaternion, it_quaternion, y));
			}
			else {
				return XMMatrixRotationQuaternion(rit_quaternion);
			}
		}();

		// ��]�̓K�p
		bone_matrix_container[index] =
			XMMatrixTranslation(-pmx_bone[index].position.x, -pmx_bone[index].position.y, -pmx_bone[index].position.z) *
			quaternion *
			XMMatrixTranslation(pmx_bone[index].position.x, pmx_bone[index].position.y, pmx_bone[index].position.z);


		/*
		// ����IK�p�̃p�����[�^���ۂ��H�Ԉ���Ă��邩��
		// �ړ��̓K�p
		auto transform = [rit, it, &motion_data, frame_num]() {
			if (it != motion_data.end()) {
				auto t = static_cast<float>(frame_num - rit->frame_num) / static_cast<float>(it->frame_num - rit->frame_num);
				return XMFLOAT3{
					rit->transform.x * (1 - t) + it->transform.x * t ,
					rit->transform.y * (1 - t) + it->transform.y * t,
					rit->transform.z * (1 - t) + it->transform.z * t,
				};
			}
			else {
				return rit->transform;
			}
		}();

		bone_matrix_container[index] *= XMMatrixTranslation(transform.x, transform.y, transform.z);
		*/
	}
}

// int��frame num
template<typename T>
std::unordered_map<std::wstring, std::vector<bone_motion_data>> get_bone_name_to_bone_motion_data(T&& vmd_data)
{
	std::unordered_map<std::wstring, std::vector<bone_motion_data>> result{};

	for (auto&& vmd : std::forward<T>(vmd_data))
	{
		// �����Ƃ��
		auto name = mmdl::ansi_to_utf16<std::wstring, std::string>(std::string{ vmd.name });

		result[name].emplace_back(static_cast<int>(vmd.frame_num), vmd.transform, vmd.quaternion,
			vmd.complement_parameter[3], vmd.complement_parameter[7], vmd.complement_parameter[11], vmd.complement_parameter[15]);
	}

	return result;
}

// �e�̉�]�A�ړ���\���s����q�ɓK�p����
template<typename T, typename U>
void recursive_aplly_parent_matrix(T& matrix_container, std::size_t current_index, XMMATRIX const& parent_matrix, U const& to_children_bone_index_container)
{
	matrix_container[current_index] *= parent_matrix;

	for (auto children_index : to_children_bone_index_container[current_index])
	{
		recursive_aplly_parent_matrix(matrix_container, children_index, matrix_container[current_index], to_children_bone_index_container);
	}
}

// �ċA�I�ɍs��������Ă�������
template<typename T, typename U>
void recursive_aplly_matrix(T& matrix_container, std::size_t current_index, XMMATRIX const& matrix, U const& to_children_bone_index_container)
{
	matrix_container[current_index] *= matrix;

	for (auto children_index : to_children_bone_index_container[current_index])
	{
		recursive_aplly_matrix(matrix_container, children_index, matrix, to_children_bone_index_container);
	}
}

// utf16�̃|�[�Y�̃f�[�^�̓ǂݍ���
inline std::vector<mmdl::vpd_data<std::wstring, XMFLOAT3, XMFLOAT4>> get_utf16_vpd_data(std::istream& in)
{
	std::vector<mmdl::vpd_data<std::wstring, XMFLOAT3, XMFLOAT4>> result{};

	auto tmp_container = mmdl::load_vpd_data<std::vector, std::string, XMFLOAT3, XMFLOAT4>(in);
	result.reserve(tmp_container.size());

	for (auto&& tmp : std::move(tmp_container))
	{
		result.emplace_back(
			mmdl::ansi_to_utf16<std::wstring, std::string>(std::move(tmp.name)),
			std::move(tmp.transform),
			std::move(tmp.quaternion)
		);
	}

	return result;
}

// �{�[���̖��O����{�[���̃C���f�b�N�X��Ԃ�unordered_map�̐���
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

// �P�F��4x4�̑傫���̃e�N�X�`���̃��\�[�X��Ԃ�
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
std::vector<dx12w::resource_and_state> get_pmx_texture_resrouce(T& device, U& command_manager, S const& pmx_texture_path, DXGI_FORMAT format, std::wstring const& directory_path)
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


// bone�s��͉�]�̂ݓK�p����Ă���
void solve_CCDIK(std::array<XMMATRIX, MAX_BONE_NUM>& bone, std::size_t root_index, std::vector<mmdl::pmx_bone< std::wstring, XMFLOAT3, std::vector>> const& pmx_bone, XMFLOAT3& target_position,
	std::vector<std::vector<std::size_t>> const& to_children_bone_index, std::size_t ik_roop_max_num, bool ideal, bool is_residual)
{
	auto target_index = pmx_bone[root_index].ik_target_bone;

	// ���[�v����IK���������Ă���
	for (std::size_t ik_roop_i = 0; ik_roop_i < static_cast<std::size_t>(pmx_bone[root_index].ik_roop_number); ik_roop_i++)
	{

		// �c����]
		// �{�[�����꒼���ɂȂ�O�ς̌v�Z���s���Ȃ��Ȃ��悤�Ȏ��Ԃ�����邽��
		// �Q�l: http://kzntov.seesaa.net/article/455959577.html
		XMVECTOR residual_rotation = XMQuaternionIdentity();

		// ���ꂼ��̃{�[���𓮂����Ă���
		for (std::size_t ik_link_i = 0; ik_link_i < pmx_bone[root_index].ik_link.size(); ik_link_i++)
		{
			if (ik_roop_i * pmx_bone[root_index].ik_link.size() + ik_link_i >= ik_roop_max_num) {
				return;
			}

			// �Ώۂ�ik_link�̃{�[��
			auto ik_link = pmx_bone[root_index].ik_link[ik_link_i];

			// ���݂̃^�[�Q�b�g�̃{�[���̈ʒu
			auto current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position), bone[target_index]);

			// �Ώۂ̃{�[���̈ʒu
			auto ik_link_bone_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[ik_link.bone].position), bone[ik_link.bone]);

			// �Ώۂ̃{�[�����猻�݂̃^�[�Q�b�g�ւ̃x�N�g��
			auto to_current_target = XMVector3Normalize(XMVectorSubtract(current_target_position, ik_link_bone_position));
			//std::cout << "ik_roop_i: " << ik_roop_i << " ik_link_i: " << ik_link_i <<
				//" to_current_target: " << to_current_target.m128_f32[0] << " " << to_current_target.m128_f32[1] << " " << to_current_target.m128_f32[2] << " " << to_current_target.m128_f32[3] << std::endl;

			// �Ώۂ̃{�[������^�[�Q�b�g�ւ̃x�N�g��
			auto to_target = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&target_position), ik_link_bone_position));
			//std::cout << "ik_roop_i: " << ik_roop_i << " ik_link_i: " << ik_link_i <<
				//" to_target: " << to_target.m128_f32[0] << " " << to_target.m128_f32[1] << " " << to_target.m128_f32[2] << " " << to_target.m128_f32[3] << std::endl;


			// �قړ����x�N�g���ɂȂ��Ă��܂����ꍇ�͊O�ς��v�Z�ł��Ȃ����ߔ�΂�
			if (XMVector3Length(XMVectorSubtract(to_current_target, to_target)).m128_f32[0] <= std::numeric_limits<float>::epsilon()) {
				continue;
			}

			// �O�ς���ъp�x�̌v�Z
			// pmx�̓��[�J�����W�n�݂����ȃp�����[�^�����邯�ǁA�Ƃ肠��������
			auto cross = XMVector3Normalize(XMVector3Cross(to_current_target, to_target));
			auto angle = XMVector3AngleBetweenVectors(to_current_target, to_target).m128_f32[0];


			//std::cout << "ik_roop_i: " << ik_roop_i << " ik_link_i: " << ik_link_i << " cross_angle_rotaion.x: " << XMQuaternionRotationMatrix(XMMatrixRotationAxis(cross, angle)).m128_f32[0] << std::endl;
			// �p�x�������l�����Ȃ����z�I�ȉ�]
			auto ideal_rotation = [&is_residual, &residual_rotation, &cross, &angle]() {
				if (is_residual)
					return XMQuaternionNormalize(XMQuaternionMultiply(residual_rotation, XMQuaternionRotationMatrix(XMMatrixRotationAxis(cross, angle))));
				else
					return XMQuaternionNormalize(XMQuaternionRotationMatrix(XMMatrixRotationAxis(cross, angle)));
			}();
			//std::cout << "ik_roop_i: " << ik_roop_i << " ik_link_i: " << ik_link_i << " ideal_rotation.x: " << ideal_rotation.m128_f32[0] << std::endl;
			//std::cout << "ideal: " << (ideal_rotation.m128_f32[3] > 0.f ? true : false) << std::endl;

			// �p�x�̐������l���������ۂ̉�]��\���s��Ɖ�]�̍s�񂪏C�����ꂽ���ǂ���
			auto [actual_rotation, is_fixed_rotaion] = [&cross, &angle, &ik_link](auto const& ideal_rotation) {

				// �������Ȃ��ꍇ���̂܂�
				if (!ik_link.min_max_angle_limit)
				{
					return std::make_pair(ideal_rotation, false);
				}
				// ����������ꍇ�͒�������
				else
				{
					auto [angle_limit_min, angle_limit_max] = ik_link.min_max_angle_limit.value();
					// �R�s�[����
					auto result = ideal_rotation;

					result.m128_f32[0] /= result.m128_f32[3];
					result.m128_f32[1] /= result.m128_f32[3];
					result.m128_f32[2] /= result.m128_f32[3];
					result.m128_f32[3] = 1.f;

					auto is_fixed_rotaion = false;

					// x���̉�]�ɂ��Ē���
					XMFLOAT3 x_axis{ 1.f,0.f,0.f };
					auto rot_limit_max_x = std::sin(angle_limit_max.x * 0.5f);
					auto rot_limit_min_x = std::sin(angle_limit_min.x * 0.5f);
					if (result.m128_f32[0] > rot_limit_max_x) {
						result.m128_f32[0] = rot_limit_max_x;
						is_fixed_rotaion = true;
					}
					else if (result.m128_f32[0] < rot_limit_min_x) {
						result.m128_f32[0] = rot_limit_min_x;
						is_fixed_rotaion = true;
					}

					// y���̉�]�ɂ��Ē���
					XMFLOAT3 y_axis{ 0.f,1.f,0.f };
					auto rot_limit_max_y = std::sin(angle_limit_max.y * 0.5f);
					auto rot_limit_min_y = std::sin(angle_limit_min.y * 0.5f);
					if (result.m128_f32[1] > rot_limit_max_y) {
						result.m128_f32[1] = rot_limit_max_y;
						is_fixed_rotaion = true;
					}
					else if (result.m128_f32[1] < rot_limit_min_y) {
						result.m128_f32[1] = rot_limit_min_y;
						is_fixed_rotaion = true;
					}

					// z���̉�]�ɂ��Ă̒���
					XMFLOAT3 z_axis{ 0.f,0.f,1.f };
					auto rot_limit_max_z = std::sin(angle_limit_max.z * 0.5f);
					auto rot_limit_min_z = std::sin(angle_limit_min.z * 0.5f);
					if (result.m128_f32[2] > rot_limit_max_z) {
						result.m128_f32[2] = rot_limit_max_z;
						is_fixed_rotaion = true;
					}
					else if (result.m128_f32[2] < rot_limit_min_z) {
						result.m128_f32[2] = rot_limit_min_z;
						is_fixed_rotaion = true;
					}

					// ���K�����čs��ɕϊ����ĕԂ�
					return std::make_pair(XMQuaternionNormalize(result), is_fixed_rotaion);
				}
			}(ideal_rotation);
			//std::cout << "ik_roop_i: " << ik_roop_i << " ik_link_i: " << ik_link_i << " actual_rotation.x: " << actual_rotation.m128_f32[0] << std::endl;

			if (ik_roop_i * pmx_bone[root_index].ik_link.size() + ik_link_i == ik_roop_max_num - 1 && ideal) {
				actual_rotation = ideal_rotation;
			}

			// �C������Ă����ꍇ�͎c���x�N�g�����X�V
			if (is_fixed_rotaion) {
				residual_rotation = XMQuaternionMultiply(ideal_rotation, XMQuaternionInverse(actual_rotation));

				auto [angle_limit_min, angle_limit_max] = ik_link.min_max_angle_limit.value();

				// �S����]���Ȃ����ɂ��Ă̗v�f��0�j���邱�Ƃ�
				// �s�v�ȐU����}���邱�Ƃ��ł���
				if (angle_limit_min.x == 0.f && angle_limit_max.x == 0.f)
					residual_rotation.m128_f32[0] = 0.f;
				if (angle_limit_min.y == 0.f && angle_limit_max.y == 0.f)
					residual_rotation.m128_f32[1] = 0.f;
				if (angle_limit_min.z == 0.f && angle_limit_max.z == 0.f)
					residual_rotation.m128_f32[2] = 0.f;

				residual_rotation = XMQuaternionNormalize(residual_rotation);
			}
			else {
				residual_rotation = XMQuaternionIdentity();
			}


			// ���_wp���S�ɉ�]����悤�ɏC��
			auto rot = XMMatrixTranslationFromVector(-ik_link_bone_position) *
				XMMatrixRotationQuaternion(actual_rotation) *
				XMMatrixTranslationFromVector(ik_link_bone_position);

			// �Ώۂ�ik_link�̃{�[����薖�[�̃{�[���ɉ�]��K�p����
			recursive_aplly_matrix(bone, ik_link.bone, rot, to_children_bone_index);

			// �^�[�Q�b�g�̃{�[���̈ʒu�̍X�V
			current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position), bone[target_index]);

			// �\���߂��Ȃ����ꍇ���[�v�𔲂���
			if (XMVector3Length(XMVectorSubtract(current_target_position, XMLoadFloat3(&target_position))).m128_f32[0] <= std::numeric_limits<float>::epsilon()) {
				break;
			}

			//std::cout << std::endl;
		}
	}
}