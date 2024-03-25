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
// リソース生成
//

// 単色の4x4の大きさのテクスチャのリソースを返す
template<typename T, typename U>
dx12w::resource_and_state get_fill_4x4_texture_resource(T& device, U& command_manager, DXGI_FORMAT format, std::uint8_t value);

// pmxのテクスチャのリソースの取得
template<typename T, typename U, typename S>
std::vector<dx12w::resource_and_state> get_pmx_texture_resource(T& device, U& command_manager, S const& pmx_texture_path, DXGI_FORMAT format, std::wstring const& directory_path);

// pmxのマテリアルのリソースの取得
template<typename T, typename U>
std::vector<dx12w::resource_and_state> get_pmx_material_resource(T& device, U const& pmx_material);


//
// pmx
//

// ボーンの名前からボーンのインデックスを返すunordered_mapの生成
template<typename T>
std::unordered_map<std::wstring_view, std::size_t> get_bone_name_to_bone_index(T const& pmx_bone);

// モーフのpmxのインデックスからモーフが格納されている配列のインデックスを返すunordered_mapの生成
template<typename T>
std::unordered_map<std::size_t, std::size_t> get_morph_index_to_morph_vertex_index(T const& pmx_morph);

// 子ボーンへのインデックスのリストを作成
template<typename T>
std::vector<std::vector<std::size_t>> get_to_children_bone_index(T const& pmx_bone);


//
// vpd、vmd
//

// vpdのデータをボーンに反映させる
template<typename T, typename U, typename S>
void set_bone_data_from_vpd(T& bone_data, U const& vpd_data, S const& bone_name_to_bone_index);

// vmdのデータを使いやすいように変換
template<typename T>
std::unordered_map<std::wstring, std::vector<bone_motion_data>> get_bone_name_to_bone_motion_data(T&& vmd_data);

// vmdのモーフのデータを使いやすいように変換
template<typename T>
std::unordered_map<std::wstring, std::vector<morph_motion_data>> get_morph_name_to_morph_motion_data(T&& vmd_data);

// vmdのデータをボーンに反映させる
template<typename T, typename U, typename S, typename R>
void set_bone_data_from_vmd(T& bone_data, U const& bone_name_to_bone_motion_data, S const& pmx_bone, R const& bone_name_to_bone_index, std::size_t frame_num);

// そのフレームのモーフのウェイトを線形補完し返す
template<typename T>
float get_morph_motion_weight(T const& morph_data, std::wstring const& morph_name, std::size_t frame_num);

//
// bone_data
//

// ボーンデータの初期化
template<typename T>
void initialize_bone_data(T& bone_data);

// 再帰的に走査し親ボーンの情報を保持させる
template<typename T, typename U, typename S>
void set_to_world_matrix(T& bone_data, U const& to_children_index, std::size_t current_index, XMMATRIX const& parent_matrix, S const& pmx_bone);

// ccdikする
void solve_CCDIK(std::vector<bone_data>& bone, std::size_t root_index, auto const& pmx_bone, XMFLOAT3& target_position,
	std::vector<std::vector<std::size_t>> const& to_children_bone_index, int debug_ik_rotation_num, int* debug_ik_rotation_counter, bool debug_check_ideal_rotation);

// 再帰的にボーンをたどっていきikの処理を行う
template<typename T, typename U, typename S>
void recursive_aplly_ik(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone, int debug_ik_rotation_num = -1, int* debug_ik_rotation_counter = nullptr, bool debug_check_ideal_rotation = false);

// 回転付与、移動付与の処理
template<typename T, typename U, typename S>
void solve_grant(T& bone_data, U const& pmx_data, std::size_t index, S const& to_children_index);

// 回転付与、移動付与の処理を再帰的にやってく
template<typename T, typename U, typename S>
void recursive_aplly_grant(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone);

// ボーンのデータからボーン行列の生成
template<typename T, typename U, typename S>
void bone_data_to_bone_matrix(T& bone_data, U& bone_matrix, S const& pmx_bone);


//
// その他
//

// ベジェ曲線の計算
inline float calc_bezier_curve(float x, float p1_x, float p1_y, float p2_x, float p2_y);

// クォータニオンのクランプ
inline bool clamp_quaternion(XMVECTOR& q, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);

// ansiからUTF16に変換
inline std::wstring ansi_to_utf16(std::string_view const& src);


//
// 以下、実装
//

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
		// 対象のボーンが存在しない場合は飛ばす
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

	// 後で検索できるようにそれぞれのモーションをフレーム番号順にソート
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

	// 後で検索しやすいようにソート
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

		// 対象のボーンが存在しない場合は飛ばす
		if (bone_name_and_index == bone_name_to_bone_index.end()) {
			continue;
		}

		auto const& bone_position = pmx_bone[bone_name_and_index->second].position;

		auto const& motion_data = motion.second;

		// 配列の末尾から検索しフレーム数より大きく、かつフレーム数に一番近いモーションデータを検索する
		auto const curr_motion_riter = std::find_if(motion_data.rbegin(), motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

		// 対象のモーションデータが存在しない場合は飛ばす
		if (curr_motion_riter == motion_data.crend()) {
			continue;
		}

		// 一つ手前のボーンのモーションデータを参照できる
		auto const prev_motion_iter = curr_motion_riter.base();

		// 回転の計算
		auto const [rotation, transform] = [curr_motion_riter, prev_motion_iter, &motion_data, frame_num]() {
			if (prev_motion_iter != motion_data.end()) {
				auto const t = static_cast<float>(frame_num - curr_motion_riter->frame_num) / static_cast<float>(prev_motion_iter->frame_num - curr_motion_riter->frame_num);

				// ベジェ曲線を利用したそれぞれのパラメータを計算
				auto const x_t = calc_bezier_curve(t, prev_motion_iter->x_a[0] / 127.f, prev_motion_iter->x_a[1] / 127.f, prev_motion_iter->x_b[0] / 127.f, prev_motion_iter->x_b[1] / 127.f);
				auto const y_t = calc_bezier_curve(t, prev_motion_iter->y_a[0] / 127.f, prev_motion_iter->y_a[1] / 127.f, prev_motion_iter->y_b[0] / 127.f, prev_motion_iter->y_b[1] / 127.f);
				auto const z_t = calc_bezier_curve(t, prev_motion_iter->z_a[0] / 127.f, prev_motion_iter->z_a[1] / 127.f, prev_motion_iter->z_b[0] / 127.f, prev_motion_iter->z_b[1] / 127.f);
				auto const r_t = calc_bezier_curve(t, prev_motion_iter->r_a[0] / 127.f, prev_motion_iter->r_a[1] / 127.f, prev_motion_iter->r_b[0] / 127.f, prev_motion_iter->r_b[1] / 127.f);

				// 線形補間
				auto const x = std::lerp(curr_motion_riter->transform.x, prev_motion_iter->transform.x, x_t);
				auto const y = std::lerp(curr_motion_riter->transform.y, prev_motion_iter->transform.y, y_t);
				auto const z = std::lerp(curr_motion_riter->transform.z, prev_motion_iter->transform.z, z_t);
				auto const transform = XMVECTOR{ x,y,z };

				// 球面補間
				auto const rotation = XMQuaternionSlerp(XMLoadFloat4(&curr_motion_riter->quaternion), XMLoadFloat4(&prev_motion_iter->quaternion), r_t);

				return std::make_pair(rotation, transform);
			}
			else {
				return std::make_pair(XMLoadFloat4(&curr_motion_riter->quaternion), XMVECTOR{ curr_motion_riter->transform.x, curr_motion_riter->transform.y, curr_motion_riter->transform.z });
			}
		}();

		// ボーンデータに反映
		bone_data[bone_name_and_index->second].rotation = rotation;
		bone_data[bone_name_and_index->second].transform = transform;
	}
}

template<typename T>
float get_morph_motion_weight(T const& morph_data, std::wstring const& morph_name, std::size_t frame_num)
{
	auto const iter = morph_data.find(morph_name);

	// 対象のモーフのアニメーションのデータがない場合
	if (iter == morph_data.end())
		return 0.f;

	auto const& motion_data = iter->second;

	// 配列の末尾から検索しフレーム数より大きく、かつフレーム数に一番近いモーションデータを検索する
	auto const curr_motion_riter = std::find_if(motion_data.rbegin(), motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

	// 対象のモーションデータが存在しない場合
	if (curr_motion_riter == motion_data.crend()) 
		return 0.f;

	// 一つ手前のボーンのモーションデータを参照
	auto const prev_motion_iter = curr_motion_riter.base();

	// 手前のモーションデータが存在しない場合
	if (prev_motion_iter == motion_data.end())
		return 0.f;

	auto const t = static_cast<float>(frame_num - curr_motion_riter->frame_num) / static_cast<float>(prev_motion_iter->frame_num - curr_motion_riter->frame_num);

	// 線形保管して重みを返す
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

	// ループしてIKを解決していく
	for (std::size_t ik_roop_i = 0; ik_roop_i < static_cast<std::size_t>(pmx_bone[root_index].ik_roop_number); ik_roop_i++)
	{

		// それぞれのボーンを動かしていく
		for (std::size_t ik_link_i = 0; ik_link_i < pmx_bone[root_index].ik_link.size(); ik_link_i++)
		{

			if (debug_ik_rotation_counter != nullptr && debug_ik_rotation_num >= 0 && *debug_ik_rotation_counter >= debug_ik_rotation_num) {
				return;
			}

			// 回転の対象であるik_linkのボーン
			auto const& ik_link = pmx_bone[root_index].ik_link[ik_link_i];

			// 対象のik_linkのボーンの座標系からワールド座標への変換
			auto const to_world =
				XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[ik_link.bone].position)) *
				XMMatrixRotationQuaternion(bone[ik_link.bone].rotation) *
				XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[ik_link.bone].position)) *
				XMMatrixTranslationFromVector(bone[ik_link.bone].transform) *
				bone[ik_link.bone].to_world;

			// ワールド座標系から対象のik_linkのボーンの座標系への変換
			// WARNING: 逆行列計算のコストは?
			auto const to_local = XMMatrixInverse(nullptr, to_world);

			// 現在のターゲットのボーンのワールド座標での位置
			auto const world_current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position),
				XMMatrixTranslationFromVector(bone[target_index].transform) *
				bone[target_index].to_world);

			// 現在のターゲットのボーンの対象のik_linkのボーンの座標系の位置
			auto const local_current_target_position = XMVector3Transform(world_current_target_position, to_local);

			// 対象のボーンのワールド座標での位置
			auto const world_ik_link_bone_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[ik_link.bone].position), to_world);

			// 対象のボーンの座標系での対象のボーンの位置
			// つまり、そのままの座標
			auto const local_ik_link_bone_position = XMLoadFloat3(&pmx_bone[ik_link.bone].position);

			// 対象のボーンの座標系でのターゲットのボーンの理想的な位置
			// （local_current_target_position を local_target_position へ近づけるのが目的）
			auto const local_target_position = XMVector3Transform(XMLoadFloat3(&target_position), to_local);

			// 対象のボーンから現在のターゲットへのベクトル
			auto const to_current_target = XMVector3Normalize(XMVectorSubtract(local_current_target_position, local_ik_link_bone_position));

			// 対象のボーンからターゲットへのベクトル
			auto const to_target = XMVector3Normalize(XMVectorSubtract(local_target_position, local_ik_link_bone_position));


			// ほぼ同じベクトルになってしまった場合は外積が計算できないため飛ばす
			if (XMVector3Length(XMVectorSubtract(to_current_target, to_target)).m128_f32[0] <= std::numeric_limits<float>::epsilon()) {
				continue;
			}

			// 外積および角度の計算
			auto const cross = XMVector3Normalize(XMVector3Cross(to_current_target, to_target));
			auto const angle = XMVector3AngleBetweenVectors(to_current_target, to_target).m128_f32[0];

			// 角度制限を考慮しない理想的な回転
			auto const ideal_rotation = XMQuaternionMultiply(XMQuaternionRotationMatrix(XMMatrixRotationAxis(cross, angle)), bone[ik_link.bone].rotation);

			// デバック用
			// 角制限を無視した場合の回転を表示するためのフラグ
			bool const use_ideal_rotation_for_debug = debug_check_ideal_rotation && debug_ik_rotation_counter != nullptr && *debug_ik_rotation_counter >= debug_ik_rotation_num - 1;

			// 角度の制限を考慮した実際の回転を表す行列と回転の行列が修正されたかどうか
			auto const actual_rotation = [&cross, &angle, &ik_link, use_ideal_rotation_for_debug](auto const& ideal_rotation) {

				// デバッグ目的で理想回転を確認するとき
				if (use_ideal_rotation_for_debug) {
					return ideal_rotation;
				}

				// 制限がない場合そのまま
				if (!ik_link.min_max_angle_limit)
				{
					return ideal_rotation;
				}
				// 制限がある場合は調整する
				else
				{
					auto const& [angle_limit_min, angle_limit_max] = ik_link.min_max_angle_limit.value();
					// コピーする
					auto result = ideal_rotation;

					// クランプ
					clamp_quaternion(result, angle_limit_min.x, angle_limit_max.x, angle_limit_min.y, angle_limit_max.y, angle_limit_min.z, angle_limit_max.z);

					return result;
				}
			}(ideal_rotation);

			// 回転を反映
			bone[ik_link.bone].rotation = actual_rotation;

			// 対象のik_linkのボーンより末端のボーンに回転を適用する
			set_to_world_matrix(bone, to_children_bone_index, ik_link.bone, bone[ik_link.bone].to_world, pmx_bone);

			// デバッグ用
			// ikの処理によって回転した回数を記録する
			if (debug_ik_rotation_counter) {
				(*debug_ik_rotation_counter)++;
			}

			// ターゲットのボーンの位置の更新
			auto const updated_world_current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position),
				XMMatrixTranslationFromVector(bone[target_index].transform) *
				bone[target_index].to_world);

			// 十分近くなった場合ループを抜ける
			if (XMVector3Length(XMVectorSubtract(updated_world_current_target_position, XMLoadFloat3(&target_position))).m128_f32[0] <= std::numeric_limits<float>::epsilon()) {
				break;
			}
		}
	}
}

template<typename T, typename U, typename S>
void recursive_aplly_ik(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone, int debug_ik_rotation_num, int* debug_ik_rotation_counter, bool debug_check_ideal_rotation)
{
	// 現在の対象のボーンがikの場合
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

	// 再帰的に子ボーンをたどっていく
	for (auto children_index : to_children_bone_index[current_index])
	{
		recursive_aplly_ik(bone, children_index, to_children_bone_index, pmx_bone, debug_ik_rotation_num, debug_ik_rotation_counter, debug_check_ideal_rotation);
	}
}


template<typename T, typename U, typename S>
void solve_grant(T& bone_data, U const& pmx_data, std::size_t index, S const& to_children_index)
{
	// 対象のボーンが回転付与の場合
	if (pmx_data[index].bone_flag_bits[static_cast<std::size_t>(bone_flag::rotation_grant)])
	{
		// 正の方向の回転
		if (pmx_data[index].grant_rate >= 0.f)
		{
			// 追加分の回転を計算
			auto const additional_rotation = XMQuaternionSlerp(XMQuaternionIdentity(), bone_data[pmx_data[index].grant_index].rotation, pmx_data[index].grant_rate);

			// 反映
			bone_data[index].rotation = XMQuaternionMultiply(bone_data[index].rotation, additional_rotation);
			set_to_world_matrix(bone_data, to_children_index, index, bone_data[index].to_world, pmx_data);
		}
		// 負の方向の回転
		else
		{
			// 割合を正にする
			auto const modified_rate = -pmx_data[index].grant_rate;
			// 回転方向も修正する
			auto const rotation_inv = XMQuaternionInverse(bone_data[pmx_data[index].grant_index].rotation);

			// 追加分の回転を計算
			auto const additional_rotation = XMQuaternionSlerp(XMQuaternionIdentity(), rotation_inv, modified_rate);

			// 反映
			bone_data[index].rotation = XMQuaternionMultiply(bone_data[index].rotation, additional_rotation);
			set_to_world_matrix(bone_data, to_children_index, index, bone_data[index].to_world, pmx_data);
		}
	}

	// 対象のボーンが移動付与の場合
	if (pmx_data[index].bone_flag_bits[static_cast<std::size_t>(bone_flag::move_grant)])
	{
		// 追加分の移動
		auto const additional_transform = XMVectorScale(bone_data[pmx_data[index].grant_index].transform, pmx_data[index].grant_rate);

		// 反映
		bone_data[index].transform += additional_transform;
		set_to_world_matrix(bone_data, to_children_index, index, bone_data[index].to_world, pmx_data);
	}
}

template<typename T, typename U, typename S>
void recursive_aplly_grant(T& bone, std::size_t current_index, U const& to_children_bone_index, S const& pmx_bone)
{
	solve_grant(bone, pmx_bone, current_index, to_children_bone_index);

	// 再帰的に子ボーンをたどっていく
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
	// ニュートン法を適用するxのtについての式
	auto f = [x, p1_x, p2_x](float t) {
		return (1.f + 3.f * p1_x - 3.f * p2_x) * t * t * t
			+ (3.f * p2_x - 6.f * p1_x) * t * t + 3.f * p1_x * t - x;
	};

	// fの導関数
	auto df = [x, p1_x, p2_x](float t) {
		return 3.f * (1.f + 3.f * p1_x - 3.f * p2_x) * t * t + 2.f * (3.f * p2_x - 6.f * p1_x) * t + 3.f * p1_x;
	};

	// 初期値
	float t = 0.5;

	// 収束するでしょ
	for (std::size_t i = 0; i < 5; i++)
	{
		t -= f(t) / df(t);
	}

	// yの値
	return 3.f * (1.f - t) * (1.f - t) * t * p1_y + 3.f * (1.f - t) * t * t * p2_y + t * t * t;
}

inline bool clamp_quaternion(XMVECTOR& q, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max)
{
	// それぞれの軸周りの回転を取得できるようにクォータニオンを修正
	q.m128_f32[0] /= q.m128_f32[3];
	q.m128_f32[1] /= q.m128_f32[3];
	q.m128_f32[2] /= q.m128_f32[3];
	q.m128_f32[3] = 1.f;

	// 実際に値が変更されたかどうか
	auto is_clamped = false;

	// x軸の回転について制限
	
	// x軸の回転を表すクオータニオンについて
	//  [1*sin(rot/2), 0*sin(rot/2), 0*sin(rot/2), cos(rot/2)]
	// =[sin(rot/2), 0, 0, cos(rot/2)]
	// w成分が1になるように正規化を行い
	// =[sin(rot/2)/cos(rot/2), 0, 0, 1]
	// =[tan(rot/2), 0, 0, 1]
	// min_rotとmax_rotをtan(rot/2)で変換してクランプ

	// tanの計算ができるように補正
	x_min = x_min < -3.14 ? -3.14 : x_min;
	x_max = x_max > 3.14 ? 3.14 : x_max;

	auto const x_min_rot = std::tan(x_min * 0.5f);
	auto const x_max_rot = std::tan(x_max * 0.5f);
	auto const x_updated = std::clamp(q.m128_f32[0], x_min_rot, x_max_rot);
	if (q.m128_f32[0] != x_updated) {
		q.m128_f32[0] = x_updated;
		is_clamped = true;
	}

	// y軸の回転について制限
	y_min = y_min < -3.14 ? -3.14 : y_min;
	y_max = y_max > 3.14 ? 3.14 : y_max;
	auto const y_min_rot = std::sin(y_min * 0.5f);
	auto const y_max_rot = std::sin(y_max * 0.5f);
	auto const y_updated = std::clamp(q.m128_f32[1], y_min_rot, y_max_rot);
	if (q.m128_f32[1] != y_updated) {
		q.m128_f32[1] = y_updated;
		is_clamped = true;
	}

	// z軸の回転について
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
	// 返還後の大きさの取得
	auto dst_size = MultiByteToWideChar(CP_ACP, 0, src.data(), -1, nullptr, 0);

	// 終点文字を追加しない
	dst_size--;

	// サイズの変更
	std::wstring result;
	result.resize(dst_size);

	// 変換
	MultiByteToWideChar(CP_ACP, 0, src.data(), -1, result.data(), dst_size);

	return result;
}


