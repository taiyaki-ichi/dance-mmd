#pragma once
#include"../external/mmd-loader/mmdl/pmx_loader_traits.hpp"
#include"../external/mmd-loader/mmdl/vmd_loader_traits.hpp"
#include"../external/mmd-loader/mmdl/vpd_loader_traits.hpp"
#include"struct.hpp"
#include"function.hpp"



//
// ローダのtraitsの実装
//

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
	// サイズを指定して構築
	static std::vector<model_vertex_data> construct(std::size_t size) {
		std::vector<model_vertex_data> result{};
		result.reserve(size);
		return result;
	}

	// 要素を末尾に追加
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

			// SDEFは実装していないので、とりあえずBDEF2とおなじで
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
	// サイズを指定して構築
	static std::vector<index> construct(std::size_t size) {
		std::vector<index> result{};
		result.reserve(size);
		return result;
	}

	// 要素を追加
	static void emplace_back(std::vector<index>& surface, std::size_t i) {
		surface.push_back(static_cast<index>(i));
	}
};

template<>
struct mmdl::pmx_texture_path_traits<std::vector<std::wstring>>
{
	using char_type = wchar_t;

	// サイズを指定して構築
	static std::vector<std::wstring> construct(std::size_t size)
	{
		std::vector<std::wstring> result{};
		result.reserve(size);

		return result;
	}

	// 要素を追加
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

	// サイズを指定して構築
	static std::pair<std::vector<material_data>, std::vector<material_data_2>> construct(std::size_t size)
	{
		std::vector<material_data> material{};
		std::vector<material_data_2> texture_material{};

		material.reserve(size);
		texture_material.reserve(size);

		return std::make_pair(std::move(material), std::move(texture_material));
	}

	// 要素を追加
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

	// サイズを指定して構築
	static std::vector<model_bone_data> construct(std::size_t size)
	{
		std::vector<model_bone_data> result{};
		result.reserve(size);
		return result;
	}

	// 要素を追加
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


template<>
struct mmdl::vmd_header_traits<vmd_header>
{
	static vmd_header construct(vmd_header_buffer const& buffer) {
		return { buffer.frame_data_num };
	}
};

template<>
struct mmdl::vmd_frame_data_traits<std::vector<vmd_frame_data>>
{
	static std::vector<vmd_frame_data> construct(std::size_t size) {
		std::vector<vmd_frame_data> result{};
		result.reserve(size);
		return result;
	}

	static void emplace_back(std::vector<vmd_frame_data>& frame_data, vmd_frame_data_buffer const& buffer)
	{
		auto name = ansi_to_utf16(buffer.name.data());

		// 追加
		frame_data.emplace_back(
			std::move(name),
			buffer.frame_num,
			XMFLOAT3{ buffer.transform[0],buffer.transform[1] ,buffer.transform[2] },
			XMFLOAT4{ buffer.quaternion[0],buffer.quaternion[1] ,buffer.quaternion[2] ,buffer.quaternion[3] },
			buffer.complement_parameter
		);
	}
};

template<>
struct mmdl::vpd_data_traits<std::vector<vpd_data>>
{
	static std::vector<vpd_data> construct(std::size_t size) {
		std::vector<vpd_data> result{};
		result.reserve(size);
		return result;
	}

	static void emplace_back(std::vector<vpd_data>& vpd_data, vpd_buffer const& buffer)
	{
		auto name = ansi_to_utf16({ buffer.name.data() });

		// 要素を追加
		vpd_data.emplace_back(std::move(name), XMFLOAT3{ buffer.transform[0],buffer.transform[1] ,buffer.transform[2] },
			XMFLOAT4{ buffer.quaternion[0],buffer.quaternion[1] ,buffer.quaternion[2] ,buffer.quaternion[3] });
	}
};

template<>
struct mmdl::pmx_morph_traits<std::pair<std::vector<vertex_morph>,std::vector<group_morph>>>
{
	using char_type = wchar_t;

	// サイズを指定して構築
	static std::pair<std::vector<vertex_morph>, std::vector<group_morph>> construct(std::size_t size)
	{
		return {};
	}

	// モーフを追加
	template<std::size_t CharBufferNum, std::size_t MorphDataNum>
	static void emplace_back(std::pair<std::vector<vertex_morph>, std::vector<group_morph>>& result, pmx_morph_buffer<char_type, CharBufferNum, MorphDataNum> const& buffer)
	{
		auto index = result.first.size() + result.second.size();

		// 頂点モーフの場合
		if (buffer.morph_type == 1)
		{
			vertex_morph vm{};
			vm.morph_index = index;
			vm.name = std::wstring(&buffer.name[0], buffer.name_size);
			vm.english_name = std::wstring(&buffer.english_name[0], buffer.english_name_size);

			for (std::size_t i = 0; i < static_cast<std::size_t>(buffer.morph_data_num); i++)
			{
				auto const& v = std::get< pmx_morph_buffer<char_type, CharBufferNum, MorphDataNum>::VERTEX_MORPH_INDEX>(buffer.morph_data[i]);
				vm.data.push_back(vertex_morph_data{XMFLOAT3{ v.offset[0],v.offset[1] ,v.offset[2] } ,static_cast<std::int32_t>(v.index) });
			}

			result.first.push_back(vm);
		}
		// グループモーフの場合
		else if (buffer.morph_type == 0)
		{
			group_morph gm{};
			gm.morph_index = index;
			gm.name = std::wstring(&buffer.name[0], buffer.name_size);
			gm.english_name = std::wstring(&buffer.english_name[0], buffer.english_name_size);

			for (std::size_t i = 0; i < static_cast<std::size_t>(buffer.morph_data_num); i++)
			{
				auto g = std::get<pmx_morph_buffer<char_type, CharBufferNum, MorphDataNum>::GROUP_MORPH_INDEX>(buffer.morph_data[i]);
				gm.data.emplace_back(g.index, g.morph_factor);
			}

			result.second.push_back(gm);
		}
	}
};

template<>
struct mmdl::vmd_morph_data_traits<std::vector<::vmd_morph_data>>
{
	static std::vector<::vmd_morph_data> construct(std::size_t size)
	{
		std::vector<::vmd_morph_data> result{};
		result.reserve(size);
		
		return result;
	}

	static void emplace_back(std::vector<::vmd_morph_data>& result, vmd_morph_data_buffer& buffer)
	{
		auto name = ansi_to_utf16(buffer.name.data());

		result.emplace_back(std::move(name), buffer.frame_num, buffer.weight);
	}
};

template<>
struct mmdl::pmx_rigidbody_traits<std::vector<rigidbody>>
{
	using char_type = wchar_t;

	// サイズを指定して構築
	static std::vector<rigidbody> construct(std::size_t size)
	{
		return {};
	}

	// 剛体を追加
	template<std::size_t CharBufferNum>
	static void emplace_back(std::vector<rigidbody>& result, pmx_rigidbody_buffer<char_type, CharBufferNum> const& buffer)
	{
		rigidbody r{};
		r.bone_index = buffer.bone_index;
		r.group = buffer.group;
		r.non_collision_group = buffer.non_collision_group;
		r.shape = buffer.shape;
		r.size = { buffer.size[0],buffer.size[1],buffer.size[2] };
		r.position = { buffer.position[0],buffer.position[1] ,buffer.position[2] };
		r.rotation = { buffer.rotation[0],buffer.rotation[1] ,buffer.rotation[2] };
		r.mass = buffer.mass;
		r.liner_damping = buffer.liner_damping;
		r.angular_damping = buffer.angular_damping;
		r.restitution = buffer.restitution;
		r.friction = buffer.friction;
		r.rigidbody_type = buffer.rigidbody_type;

		result.push_back(r);
	}
};