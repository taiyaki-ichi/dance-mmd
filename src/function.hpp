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
#include<tuple>

using namespace DirectX;

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

// 再帰的に行列をかけていくだけ
template<typename T, typename U>
void recursive_aplly_matrix(T& matrix_container, std::size_t current_index, XMMATRIX const& matrix, U const& to_children_bone_index_container)
{
	matrix_container[current_index] *= matrix;

	for (auto children_index : to_children_bone_index_container[current_index])
	{
		recursive_aplly_matrix(matrix_container, children_index, matrix, to_children_bone_index_container);
	}
}

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

template<typename T, typename U, typename S, typename R>
void set_bone_matrix_from_vmd(T& bone_matrix_container, U const& bone_name_to_bone_motion_data, S const& pmx_bone, R const& bone_name_to_bone_index, std::size_t frame_num)
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
		if (curr_motion_riter == motion_data.crend()){
			continue;
		}

		// 一つ手前のボーンのモーションデータを参照できる
		auto const prev_motion_iter = curr_motion_riter.base();

		// 回転の計算
		auto const [rotation,transform] = [curr_motion_riter, prev_motion_iter, &motion_data, frame_num]() {
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
				auto const transform = XMMatrixTranslation(x, y, z);

				// 球面補間
				auto const rotation = XMMatrixRotationQuaternion(XMQuaternionSlerp(XMLoadFloat4(&curr_motion_riter->quaternion), XMLoadFloat4(&prev_motion_iter->quaternion), r_t));

				return std::make_pair(rotation, transform);
			}
			else {
				return std::make_pair(XMMatrixRotationQuaternion(XMLoadFloat4(&curr_motion_riter->quaternion)), XMMatrixTranslation(curr_motion_riter->transform.x, curr_motion_riter->transform.y, curr_motion_riter->transform.z));
			}
		}();

		// 原点を中心とした回転
		auto const rotation_origin =
			XMMatrixTranslation(-bone_position.x, -bone_position.y, -bone_position.z) *
			rotation *
			XMMatrixTranslation(bone_position.x, bone_position.y, bone_position.z);

		// ボーン行列に反映
		bone_matrix_container[bone_name_and_index->second] = rotation_origin * transform;
	}
}

template<typename T, typename U, typename S, typename R>
void set_bone_matrix_from_vmd_2(T& bone_matrix_container, U const& bone_name_to_bone_motion_data, S const pmx_bone, R& bone_name_to_bone_index, std::size_t frame_num)
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

		// 一つ手前のボーンのモーションデータを参照できる
		auto it = rit.base();

		auto quaternion = [rit, it, &motion_data, frame_num]() {
			XMVECTOR rit_quaternion = XMLoadFloat4(&rit->quaternion);
			if (it != motion_data.end()) {
				auto t = static_cast<float>(frame_num - rit->frame_num) / static_cast<float>(it->frame_num - rit->frame_num);
				auto y = calc_bezier_curve(t, it->r_a[0] / 127.f, it->r_a[1] / 127.f, it->r_b[0] / 127.f, it->r_b[1] / 127.f);
				XMVECTOR it_quaternion = XMLoadFloat4(&it->quaternion);
				return XMMatrixRotationQuaternion(XMQuaternionSlerp(rit_quaternion, it_quaternion, y));
			}
			else {
				return XMMatrixRotationQuaternion(rit_quaternion);
			}
		}();

		// 回転の適用
		bone_matrix_container[index] =
			XMMatrixTranslation(-pmx_bone[index].position.x, -pmx_bone[index].position.y, -pmx_bone[index].position.z) *
			quaternion *
			XMMatrixTranslation(pmx_bone[index].position.x, pmx_bone[index].position.y, pmx_bone[index].position.z);

		// これIK用のパラメータっぽい？間違っているかも
		// 移動の適用
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
	}
}

// フレーム番号から対象のボーンの移動ベクトルを求める
template<typename T, typename U, typename S, typename R>
XMMATRIX calc_transfom_matrix(T& bone_matrix_container, U& bone_name_to_bone_motion_data, R& bone_name_to_bone_index,
	std::size_t frame_num, S const& to_children_bone_index_container, wchar_t const* bone_name)
{
	auto& const center_motion_data = bone_name_to_bone_motion_data[bone_name];
	auto center_index = bone_name_to_bone_index[bone_name];

	auto rit = std::find_if(center_motion_data.rbegin(), center_motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

	if (rit == center_motion_data.rend())
	{
		return XMMatrixIdentity();
	}

	// 一つ手前のボーンのモーションデータを参照できる
	auto it = rit.base();

	auto transform = [rit, it, &center_motion_data, frame_num]() {

		if (it != center_motion_data.end()) {
			auto t = static_cast<float>(frame_num - rit->frame_num) / static_cast<float>(it->frame_num - rit->frame_num);

			auto x_t = calc_bezier_curve(t, it->x_a[0] / 127.f, it->x_a[1] / 127.f, it->x_b[0] / 127.f, it->x_b[1] / 127.f);
			auto y_t = calc_bezier_curve(t, it->y_a[0] / 127.f, it->y_a[1] / 127.f, it->y_b[0] / 127.f, it->y_b[1] / 127.f);
			auto z_t = calc_bezier_curve(t, it->z_a[0] / 127.f, it->z_a[1] / 127.f, it->z_b[0] / 127.f, it->z_b[1] / 127.f);

			// 回転じゃあないし、球面補間の必要はなさそう
			auto x = std::lerp(rit->transform.x, it->transform.x, x_t);
			auto y = std::lerp(rit->transform.y, it->transform.y, y_t);
			auto z = std::lerp(rit->transform.z, it->transform.z, z_t);

			// std::cout << x << " " << y << " " << z << std::endl;
			return XMMatrixTranslation(x, y, z);

		}
		else {
			return XMMatrixTranslation(rit->transform.x, rit->transform.y, rit->transform.z);
		}
	}();

	return transform;
}

// フレーム番号から回転と移動の行列を取得する
template<typename T, typename U, typename S, typename R>
std::pair<XMMATRIX, XMMATRIX> calc_rotation_and_transfom_matrix(T& bone_matrix_container, U& bone_name_to_bone_motion_data, R& bone_name_to_bone_index,
	std::size_t frame_num, S const& to_children_bone_index_container, wchar_t const* bone_name)
{
	auto& const center_motion_data = bone_name_to_bone_motion_data[bone_name];
	auto center_index = bone_name_to_bone_index[bone_name];

	auto rit = std::find_if(center_motion_data.rbegin(), center_motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

	if (rit == center_motion_data.rend())
	{
		return std::make_pair(XMMatrixIdentity(), XMMatrixIdentity());
	}

	// 一つ手前のボーンのモーションデータを参照できる
	auto it = rit.base();

	if (it != center_motion_data.end()) {
		auto const t = static_cast<float>(frame_num - rit->frame_num) / static_cast<float>(it->frame_num - rit->frame_num);

		auto const x_t = calc_bezier_curve(t, it->x_a[0] / 127.f, it->x_a[1] / 127.f, it->x_b[0] / 127.f, it->x_b[1] / 127.f);
		auto const y_t = calc_bezier_curve(t, it->y_a[0] / 127.f, it->y_a[1] / 127.f, it->y_b[0] / 127.f, it->y_b[1] / 127.f);
		auto const z_t = calc_bezier_curve(t, it->z_a[0] / 127.f, it->z_a[1] / 127.f, it->z_b[0] / 127.f, it->z_b[1] / 127.f);
		auto const r_t = calc_bezier_curve(t, it->r_a[0] / 127.f, it->r_a[1] / 127.f, it->r_b[0] / 127.f, it->r_b[1] / 127.f);

		// 線形補間
		auto const x = std::lerp(rit->transform.x, it->transform.x, x_t);
		auto const y = std::lerp(rit->transform.y, it->transform.y, y_t);
		auto const z = std::lerp(rit->transform.z, it->transform.z, z_t);
		auto const transform = XMMatrixTranslation(x, y, z);

		// 球面補間
		auto const rotation = XMMatrixRotationQuaternion(XMQuaternionSlerp(XMLoadFloat4(&rit->quaternion), XMLoadFloat4(&it->quaternion), r_t));

		return std::make_pair(rotation, transform);

	}
	else {
		return std::make_pair(XMMatrixRotationQuaternion(XMLoadFloat4(&rit->quaternion)), XMMatrixTranslation(rit->transform.x, rit->transform.y, rit->transform.z));
	}

}

// フレーム番号から対象のボーンの移動ベクトルを求める
template<typename T, typename U, typename S, typename R>
std::tuple<float, float, float> calc_transfom_matrix_2(T& bone_matrix_container, U& bone_name_to_bone_motion_data, R& bone_name_to_bone_index,
	std::size_t frame_num, S const& to_children_bone_index_container, wchar_t const* bone_name)
{
	auto& const center_motion_data = bone_name_to_bone_motion_data[bone_name];
	auto center_index = bone_name_to_bone_index[bone_name];

	auto rit = std::find_if(center_motion_data.rbegin(), center_motion_data.rend(), [frame_num](auto const& m) {return m.frame_num < frame_num; });

	if (rit == center_motion_data.rend())
	{
		return std::tuple<float, float, float>{ 0.f, 0.f, 0.f };
	}

	// 一つ手前のボーンのモーションデータを参照できる
	auto it = rit.base();

	auto transform = [rit, it, &center_motion_data, frame_num]() {

		if (it != center_motion_data.end()) {
			auto t = static_cast<float>(frame_num - rit->frame_num) / static_cast<float>(it->frame_num - rit->frame_num);

			auto x_t = calc_bezier_curve(t, it->x_a[0] / 127.f, it->x_a[1] / 127.f, it->x_b[0] / 127.f, it->x_b[1] / 127.f);
			auto y_t = calc_bezier_curve(t, it->y_a[0] / 127.f, it->y_a[1] / 127.f, it->y_b[0] / 127.f, it->y_b[1] / 127.f);
			auto z_t = calc_bezier_curve(t, it->z_a[0] / 127.f, it->z_a[1] / 127.f, it->z_b[0] / 127.f, it->z_b[1] / 127.f);

			// 回転じゃあないし、球面補間の必要はなさそう
			auto x = std::lerp(rit->transform.x, it->transform.x, x_t);
			auto y = std::lerp(rit->transform.y, it->transform.y, y_t);
			auto z = std::lerp(rit->transform.z, it->transform.z, z_t);

			return std::tuple<float, float, float>{ x, y, z };

		}
		else {
			return std::tuple<float, float, float>{ rit->transform.x, rit->transform.y, rit->transform.z };
		}
	}();

	return transform;
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

		result[name].emplace_back(static_cast<int>(vmd.frame_num), vmd.transform, vmd.quaternion,
			std::array<char, 2>{vmd.complement_parameter[3], vmd.complement_parameter[7] }, std::array<char, 2>{vmd.complement_parameter[11], vmd.complement_parameter[15]},
			std::array<char, 2>{ vmd.complement_parameter[0], vmd.complement_parameter[4] }, std::array<char, 2>{ vmd.complement_parameter[8], vmd.complement_parameter[12] },
			std::array<char, 2>{ vmd.complement_parameter[1], vmd.complement_parameter[5] }, std::array<char, 2>{ vmd.complement_parameter[9], vmd.complement_parameter[13] },
			std::array<char, 2>{ vmd.complement_parameter[2], vmd.complement_parameter[6] }, std::array<char, 2> { vmd.complement_parameter[10], vmd.complement_parameter[14] }
		);
	}

	return result;
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


// クォータニオンをクランプする
inline bool clamp_quaternion(XMVECTOR& q, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max)
{
	// それぞれの軸周りの回転を取得できるようにクォータニオンを修正
	q.m128_f32[0] /= q.m128_f32[3];
	q.m128_f32[1] /= q.m128_f32[3];
	q.m128_f32[2] /= q.m128_f32[3];
	q.m128_f32[3] = 1.f;

	// 実際に値が変更されたかどうか
	auto is_clamped = false;

	// x軸の回転について
	auto const x_min_rot = std::sin(x_min * 0.5f);
	auto const x_max_rot = std::sin(x_max * 0.5f);
	auto const x_updated = std::clamp(q.m128_f32[0], x_min_rot, x_max_rot);
	if (q.m128_f32[0] != x_updated) {
		q.m128_f32[0] = x_updated;
		is_clamped = true;
	}

	// y軸の回転について
	auto const y_min_rot = std::sin(y_min * 0.5f);
	auto const y_max_rot = std::sin(y_max * 0.5f);
	auto const y_updated = std::clamp(q.m128_f32[1], y_min_rot, y_max_rot);
	if (q.m128_f32[1] != y_updated) {
		q.m128_f32[1] = y_updated;
		is_clamped = true;
	}

	// z軸の回転について
	auto const z_min_rot = std::sin(z_min * 0.5f);
	auto const z_max_rot = std::sin(z_max * 0.5f);
	auto const z_updated = std::clamp(q.m128_f32[2], z_min_rot, z_max_rot);
	if (q.m128_f32[2] != z_updated) {
		q.m128_f32[2] = z_updated;
		is_clamped = true;
	}

	XMQuaternionNormalize(q);

	return is_clamped;
}


// bone行列は回転のみ適用されている
void solve_CCDIK(std::array<XMMATRIX, MAX_BONE_NUM>& bone, std::size_t root_index, std::vector<mmdl::pmx_bone< std::wstring, XMFLOAT3, std::vector>> const& pmx_bone, XMFLOAT3& target_position,
	std::vector<std::vector<std::size_t>> const& to_children_bone_index, std::size_t ik_bone_rotation_num, bool check_ideal_rotation)
{
	auto target_index = pmx_bone[root_index].ik_target_bone;

	// ループしてIKを解決していく
	for (std::size_t ik_roop_i = 0; ik_roop_i < static_cast<std::size_t>(pmx_bone[root_index].ik_roop_number); ik_roop_i++)
	{

		// それぞれのボーンを動かしていく
		for (std::size_t ik_link_i = 0; ik_link_i < pmx_bone[root_index].ik_link.size(); ik_link_i++)
		{
			if (ik_roop_i * pmx_bone[root_index].ik_link.size() + ik_link_i >= ik_bone_rotation_num) {
				return;
			}

			// 回転の対象であるik_linkのボーン
			auto const& ik_link = pmx_bone[root_index].ik_link[ik_link_i];

			// 対象のik_linkのボーンの座標系からワールド座標への変換
			auto const& to_world = bone[ik_link.bone];
			// ワールド座標系から対象のik_linkのボーンの座標系への変換
			// WARNING: 逆行列計算のコストは?
			auto const to_local = XMMatrixInverse(nullptr, to_world);

			// 現在のターゲットのボーンのワールド座標での位置
			auto const world_current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position), bone[target_index]);

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
			auto const ideal_rotation = XMQuaternionNormalize(XMQuaternionRotationMatrix(XMMatrixRotationAxis(cross, angle)));

			// デバック用
			// 角制限を無視した場合の回転を表示するためのフラグ
			bool const use_ideal_rotation_for_debug = check_ideal_rotation && ik_roop_i * pmx_bone[root_index].ik_link.size() + ik_link_i == ik_bone_rotation_num - 1;

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
					clamp_quaternion(result,angle_limit_min.x, angle_limit_max.x, angle_limit_min.y, angle_limit_max.y, angle_limit_min.z, angle_limit_max.z);

					return result;
				}
			}(ideal_rotation);

			// ローカル座標での回転を表すように行列を作成
			auto const rotaion = to_local *
				XMMatrixTranslationFromVector(-local_ik_link_bone_position) *
				XMMatrixRotationQuaternion(actual_rotation) *
				XMMatrixTranslationFromVector(local_ik_link_bone_position)
				* to_world;

			// 対象のik_linkのボーンより末端のボーンに回転を適用する
			recursive_aplly_matrix(bone, ik_link.bone, rotaion, to_children_bone_index);

			// ターゲットのボーンの位置の更新
			auto const updated_world_current_target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_index].position), bone[target_index]);

			// 十分近くなった場合ループを抜ける
			if (XMVector3Length(XMVectorSubtract(updated_world_current_target_position, XMLoadFloat3(&target_position))).m128_f32[0] <= std::numeric_limits<float>::epsilon()) {
				break;
			}
		}
	}
}

// 再帰的にボーンをたどっていきikの処理を行う
template<typename T,typename U,typename S>
void recursive_aplly_ik(T& bone, std::size_t current_index, U const& to_children_bone_index,S const& pmx_bone)
{
	// 現在の対象のボーンがikの場合
	if (pmx_bone[current_index].bone_flag_bits[static_cast<std::size_t>(mmdl::bone_flag::ik)])
	{
		auto const target_bone_index = pmx_bone[current_index].ik_target_bone;
		auto const target_position = XMVector3Transform(XMLoadFloat3(&pmx_bone[target_bone_index].position), bone[current_index]);

		// TODO:
		XMFLOAT3 float3;
		XMStoreFloat3(&float3, target_position);

		solve_CCDIK(bone, current_index, pmx_bone, float3, to_children_bone_index, -1, false);
	}

	// 再帰的に子ボーンをたどっていく
	for (auto children_index : to_children_bone_index[current_index])
	{
		recursive_aplly_ik(bone, children_index, to_children_bone_index, pmx_bone);
	}
}