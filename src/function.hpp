#pragma once
#include<utility>
#include<vector>
#include<limits>
#include<DirectXMath.h>
#include<unordered_map>
#include<string>
#include"../external/mmd-loader/mmdl/vpd_loader.hpp"
#include"struct.hpp"

using namespace DirectX;

// 子ボーンへのインデックスのリストを作成
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

		// 回転の適用
		XMVECTOR quaternion_vector = XMLoadFloat4(&vpd.quaternion);
		bone_matrix_container[index] =
			XMMatrixTranslation(-pmx_bone[index].position.x, -pmx_bone[index].position.y, -pmx_bone[index].position.z) *
			XMMatrixRotationQuaternion(quaternion_vector) *
			XMMatrixTranslation(pmx_bone[index].position.x, pmx_bone[index].position.y, pmx_bone[index].position.z);

		// これIK用のパラメータっぽい？間違っているかも
		// 移動の適用
		bone_matrix_container[index] *= XMMatrixTranslation(vpd.transform.x, vpd.transform.y, vpd.transform.z);
	}
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

		// 回転の適用
		XMVECTOR quaternion_vector = XMLoadFloat4(&rit->quaternion);
		bone_matrix_container[index] =
			XMMatrixTranslation(-pmx_bone[index].position.x, -pmx_bone[index].position.y, -pmx_bone[index].position.z) *
			XMMatrixRotationQuaternion(quaternion_vector) *
			XMMatrixTranslation(pmx_bone[index].position.x, pmx_bone[index].position.y, pmx_bone[index].position.z);


		// これIK用のパラメータっぽい？間違っているかも
		// 移動の適用
		// bone_matrix_container[index] *= XMMatrixTranslation(vmd.transform.x, vmd.transform.y, vmd.transform.z);
	}
}

// intはframe num
template<typename T>
std::unordered_map<std::wstring, std::vector<bone_motion_data>> get_bone_name_to_bone_motion_data(T&& vmd_data)
{
	std::unordered_map<std::wstring, std::vector<bone_motion_data>> result{};

	for (auto&& vmd : std::forward<T>(vmd_data))
	{
		// ちゃんとやる
		auto name = mmdl::ansi_to_utf16<std::wstring, std::string>(std::string{ vmd.name });

		result[name].emplace_back(static_cast<int>(vmd.frame_num), vmd.transform, vmd.quaternion);
	}

	return result;
}

// 親の回転、移動を表す行列を子に適用する
template<typename T, typename U>
void recursive_aplly_parent_matrix(T& matrix_container, std::size_t current_index, XMMATRIX const& parent_matrix, U const& to_children_bone_index_container)
{
	matrix_container[current_index] *= parent_matrix;

	for (auto children_index : to_children_bone_index_container[current_index])
	{
		recursive_aplly_parent_matrix(matrix_container, children_index, matrix_container[current_index], to_children_bone_index_container);
	}
}

// utf16のポーズのデータの読み込み
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

// ボーンの名前からボーンのインデックスを返すunordered_mapの生成
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

// 単色の4x4の大きさのテクスチャのリソースを返す
template<typename T, typename U>
dx12w::resource_and_state get_fill_4x4_texture_resource(T& device, U& command_manager, DXGI_FORMAT format, std::uint8_t value)
{
	// 大きさは4x4
	auto result = dx12w::create_commited_texture_resource(device.get(),
		format, 4, 4, 2, 1, 1, D3D12_RESOURCE_FLAG_NONE);

	auto const dst_desc = result.first->GetDesc();

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT src_footprint;
	UINT64 src_total_bytes;
	device->GetCopyableFootprints(&dst_desc, 0, 1, 0, &src_footprint, nullptr, nullptr, &src_total_bytes);

	auto src_texture_resorce = dx12w::create_commited_upload_buffer_resource(device.get(), src_total_bytes);
	std::uint8_t* tmp = nullptr;
	src_texture_resorce.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));

	// 指定の値をセットする
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

		// 毎回excute呼び出すのはどうかと思う
		// けど、楽なのでとりあえず
		command_manager.reset_list(0);
		dx12w::resource_barrior(command_manager.get_list(), dst_texture_resource, D3D12_RESOURCE_STATE_COPY_DEST);
		command_manager.get_list()->CopyTextureRegion(&dst_texture_copy_location, 0, 0, 0, &src_texture_copy_location, nullptr);
		dx12w::resource_barrior(command_manager.get_list(), dst_texture_resource, D3D12_RESOURCE_STATE_COMMON);
		command_manager.get_list()->Close();
		command_manager.excute();
		command_manager.signal();
		command_manager.wait(0);

		// TODO: サイズ分かってるからreserveするように変更する
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