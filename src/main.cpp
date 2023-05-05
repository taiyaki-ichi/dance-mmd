#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"../external/imgui/imgui.h"
#include"../external/imgui/imgui_impl_dx12.h"
#include"../external/imgui/imgui_impl_win32.h"
#include"../external/bullet3/src/btBulletCollisionCommon.h"
#include"../external/bullet3/src/btBulletDynamicsCommon.h"
#include"../external/mmd-loader/mmdl/pmx_loader.hpp"
#include"../external/mmd-loader/mmdl/vpd_loader.hpp"
#include"../external/mmd-loader/mmdl/vmd_loader.hpp"
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include"../external/stb/stb_image.h"
#include"function.hpp"
#include"loader_traits.hpp"
#include"bullet_world.hpp"
#include"bullet_rigidbody.hpp"
#include"bullet_debug_draw.hpp"
#include<fstream>
#include<algorithm>
#include<numeric>
#include<unordered_map>
#include<DirectXMath.h>
#include<chrono>


using namespace DirectX;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Imgui用の処理
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main()
{
	auto hwnd = dx12w::create_window(L"dance-mmd", WINDOW_WIDTH, WINDOW_HEIGHT, wnd_proc);

	auto device = dx12w::create_device();
	auto command_manager = dx12w::create_command_manager<COMMAND_ALLOCATOR_NUM>(device.get());
	auto swap_chain = dx12w::create_swap_chain(command_manager.get_queue(), hwnd, FRAME_BUFFER_FORMAT, FRAME_BUFFER_NUM);


	//
	// pmxの読み込み
	//

	// file_nameはpmxファイルへのパス
	// directory_pathはファイルのフォルダへのパス
	//wchar_t const* file_path = L"E:素材/原神/パイモン/派蒙.pmx";
	//wchar_t const* directory_path = L"E:素材/原神/パイモン/";
	wchar_t const* file_path = L"E:素材/原神/アンバー/安柏.pmx";
	wchar_t const* directory_path = L"E:素材/原神/アンバー/";
	//wchar_t const* file_path = L"E:素材/キズナアイ/KizunaAI_ver1.01/kizunaai/kizunaai.pmx";
	//wchar_t const* directory_path = L"E:素材/キズナアイ/KizunaAI_ver1.01/kizunaai/";
	//wchar_t const* file_path = L"E:素材/ホロライブ/ときのそら公式mmd_ver2.1/ときのそら.pmx";
	//wchar_t const* directory_path = L"E:素材/ホロライブ/ときのそら公式mmd_ver2.1/";
	//wchar_t const* file_path = L"E:素材/ホロライブ/Laplus_220516_1/Laplus/PMX/Laplus_220516_1.pmx";
	//wchar_t const* directory_path = L"E:素材/ホロライブ/Laplus_220516_1/Laplus/sourceimages/";

	std::ifstream file{ file_path ,std::ios::binary };
	auto const pmx_header = mmdl::load_header<model_header_data>(file);
	auto const pmx_info = mmdl::load_info<model_info_data>(file);
	auto const pmx_vertex = mmdl::load_vertex<std::vector<model_vertex_data>>(file, pmx_header.add_uv_number, pmx_header.bone_index_size);
	auto const pmx_surface = mmdl::load_surface<std::vector<index>>(file, pmx_header.vertex_index_size);
	auto const pmx_texture_path = mmdl::load_texture_path<std::vector<std::wstring>>(file);
	auto const [pmx_material, pmx_material_2] = mmdl::load_material<std::pair<std::vector<material_data>, std::vector<material_data_2>>>(file, pmx_header.texture_index_size);
	auto const pmx_bone = mmdl::load_bone<std::vector<model_bone_data>>(file, pmx_header.bone_index_size);
	auto const [pmx_vertex_morph, pmx_group_morph] = mmdl::load_morph<std::pair<std::vector<vertex_morph>, std::vector<group_morph>>>(file, pmx_header.vertex_index_size, pmx_header.bone_index_size, pmx_header.material_index_size, pmx_header.morph_index_size);
	mmdl::load_display_frame(file, pmx_header.bone_index_size, pmx_header.morph_index_size);
	auto const pmx_rigidbody = mmdl::load_rigidbody<std::vector<rigidbody>>(file, pmx_header.bone_index_size);
	auto const pmx_joint = mmdl::load_joint<std::vector<joint>>(file, pmx_header.rigid_body_index_size);
	file.close();


	//
	// vpdの読み込み
	//

	//wchar_t const* pose_file_path = L"../../../3dmodel/ポーズ25/4.vpd";
	wchar_t const* pose_file_path = L"../../../3dmodel/Pose Pack 6 - Snorlaxin/9.vpd";
	std::ifstream pose_file{ pose_file_path };
	auto vpd_data = mmdl::load_vpd_data<std::vector<::vpd_data>>(pose_file);
	pose_file.close();


	//
	// vmdの読み込み
	//

	//const wchar_t* motion_file_path = L"../../../3dmodel/スイマジ/sweetmagic-right.vmd";
	const wchar_t* motion_file_path = L"../../../3dmodel/サラマンダー モーション(short)/サラマンダー モーション(short).vmd";
	std::ifstream motion_file{ motion_file_path,std::ios::binary };

	auto vmd_header = mmdl::load_vmd_header<::vmd_header>(motion_file);
	auto vmd_frame_data = mmdl::load_vmd_frame_data<std::vector<::vmd_frame_data>>(motion_file, vmd_header.frame_data_num);
	auto vmd_morph_data = mmdl::load_vmd_morph_data<std::vector<::vmd_morph_data>>(motion_file);
	motion_file.close();

	//
	// クリアバリュー
	// リソースの生成時に必要
	//

	D3D12_CLEAR_VALUE frame_buffer_clear_value{
		.Format = FRAME_BUFFER_FORMAT,
		.Color = { 0.5f,0.5f,0.5f,1.f },
	};

	D3D12_CLEAR_VALUE depth_clear_value{
		.Format = DEPTH_BUFFER_FORMAT,
		.DepthStencil{.Depth = 1.f}
	};


	//
	// リソース
	//

	// FRAME_BUFFER_NUMの数のフレームバッファ
	auto frame_buffer_resource = [&swap_chain]() {
		std::array<dx12w::resource_and_state, FRAME_BUFFER_NUM> result{};

		// スワップチェーンからフレームバッファを取得
		for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
		{
			ID3D12Resource* tmp = nullptr;
			swap_chain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&tmp));
			result[i] = std::make_pair(dx12w::release_unique_ptr<ID3D12Resource>{tmp}, D3D12_RESOURCE_STATE_COMMON);
		}

		return result;
	}();

	// デプスバッファ
	auto depth_buffer = dx12w::create_commited_texture_resource(device.get(), DEPTH_BUFFER_FORMAT, WINDOW_WIDTH, WINDOW_HEIGHT,
		2, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depth_clear_value);

	// pmxの頂点バッファ
	auto pmx_vertex_buffer_resource = [&device, &pmx_vertex]() {
		auto result = dx12w::create_commited_upload_buffer_resource(device.get(), sizeof(model_vertex_data) * pmx_vertex.size());

		model_vertex_data* tmp = nullptr;
		result.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		std::copy(pmx_vertex.begin(), pmx_vertex.end(), tmp);
		result.first->Unmap(0, nullptr);

		return result;
	}();

	// pmxのインデックスのバッファ
	auto pmx_index_buffer_resource = [&device, &pmx_surface]() {
		auto result = dx12w::create_commited_upload_buffer_resource(device.get(), sizeof(index) * pmx_surface.size());

		index* tmp = nullptr;
		result.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		std::copy(pmx_surface.begin(), pmx_surface.end(), tmp);
		result.first->Unmap(0, nullptr);

		return result;
	}();

	// model_dataをマップするリソース
	auto model_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(model_data), 256));

	// camera_dataをマップするリソース
	auto camera_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(camera_data), 256));

	// direction_light_dataをマップする用のリソース
	auto direction_light_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(direction_light_data), 256));

	// pmxを描画する頂点シェーダ
	auto pmx_vertex_shader = []() {
		std::ifstream shader_file{ L"shader/VertexShader.cso",std::ios::binary };
		return dx12w::load_blob(shader_file);
	}();

	// pmxを描画するピクセルシェーダ
	auto pmx_index_shader = []() {
		std::ifstream shader_file{ L"shader/PixelShader.cso",std::ios::binary };
		return dx12w::load_blob(shader_file);
	}();

	// 4x4の白いテクスチャ
	auto white_texture_resource = get_fill_4x4_texture_resource(device, command_manager, PMX_TEXTURE_FORMAT, 255);

	// 4x4の黒いテクスチャ
	auto black_texture_resource = get_fill_4x4_texture_resource(device, command_manager, PMX_TEXTURE_FORMAT, 0);

	// テクスチャのリソース
	auto pmx_texture_resrouce = get_pmx_texture_resource(device, command_manager, pmx_texture_path, PMX_TEXTURE_FORMAT, directory_path);

	// マテリアルのリソース
	auto pmx_material_resource = get_pmx_material_resource(device, pmx_material);


	//
	// ディスクリプタヒープとビュー
	//

	// フレームバッファに描画する時のレンダーターゲットを指定する用のデスクリプタヒープ
	auto frame_buffer_descriptor_heap_RTV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_NUM);
	// FRAME_BUFFER_NUMの数のビューを作成
	for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
		dx12w::create_texture2D_RTV(device.get(), frame_buffer_descriptor_heap_RTV.get_CPU_handle(i), frame_buffer_resource[i].first.get(), FRAME_BUFFER_FORMAT, 0, 0);

	// フレームバッファに描画する時のデプスバッファを指定する用のデスクリプタヒープ
	auto depth_buffer_descriptor_heap_DSV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	dx12w::create_texture2D_DSV(device.get(), depth_buffer_descriptor_heap_DSV.get_CPU_handle(0), depth_buffer.first.get(), DEPTH_BUFFER_FORMAT, 0);

	// pmxのディスクリプタテーブル
	// ルートシグネチャ生成時に使用するが、デスクリプタの大きさを決定する事にも利用する
	std::vector<std::vector<dx12w::descriptor_range_type>> const pmx_descriptor_table{

		// 1つめのディスクリプタテーブル
		// マテリアルに依存しない共通のビュー
		{
			// モデルのデータ、カメラのデータ、ライトのデータの3つの定数バッファ
			{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,3}
		},

		// 2つめのディスクリプタテーブル
		// マテリアルに依存したそれぞれのビュー
		{
			// マテリアルのテクスチャ
			{D3D12_DESCRIPTOR_RANGE_TYPE_SRV},
			// ディフーズなどの情報
			{D3D12_DESCRIPTOR_RANGE_TYPE_CBV},
			// 乗算スフィアマップ、加算スフィアマップ、トゥーンの3つのシェーダリソースビュー
			{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3}
		}
	};

	// pmxを描画する時の共通のビューの数とマテリアルごとにことなるビューの数
	UINT const pmx_common_view_num = dx12w::calc_descriptor_num(pmx_descriptor_table[0]);
	UINT const pmx_material_view_num = dx12w::calc_descriptor_num(pmx_descriptor_table[1]);

	// pmxを描画する用のデスクリプタヒープ
	auto pmx_descriptor_heap_CBV_SRV_UAV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		pmx_common_view_num + pmx_material_view_num * pmx_material.size());
	// 1.mode_dataのビュー
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(0), model_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(model_data), 256));
	// 2.カメラのデータのビュー
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(1), camera_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(camera_data), 256));
	// 3.ライトのデータのビュー
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(2), direction_light_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(direction_light_data), 256));
	// マテリアルごとに異なるビューを作成してく
	for (std::size_t i = 0; i < pmx_material.size(); i++)
	{
		// 4.テクスチャのビュー
		dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 0),
			pmx_texture_resrouce[pmx_material_2[i].texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		// 5.ディフューズカラーなどの情報があるmaterial_dataのビュー
		dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 1),
			pmx_material_resource[i].first.get(), dx12w::alignment<UINT64>(sizeof(material_data), 256));

		// 6.乗算スフィアマップのビュー
		// 乗算スフィアマップを使用する場合
		if (pmx_material_2[i].sphere_mode == sphere_mode::sph)
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 2),
				pmx_texture_resrouce[pmx_material_2[i].sphere_texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// しない場合は白色のテクスチャ
		else
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 2),
				white_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// 7.加算スフィアマップのビュー
		// 加算スフィアマップを使用する場合
		if (pmx_material_2[i].sphere_mode == sphere_mode::spa)
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 3),
				pmx_texture_resrouce[pmx_material_2[i].sphere_texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// しない場合は黒色のテクスチャ
		else {
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 3),
				black_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// 8.トゥーンテクスチャのビュー
		// 非共有のトゥーン
		// インデックスに無効な値が入っている場合があるので無視する
		if (pmx_material_2[i].toon_type == toon_type::unshared && pmx_material_2[i].toon_texture < pmx_texture_path.size())
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 4),
				pmx_texture_resrouce[pmx_material_2[i].toon_texture].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// それ以外は白色のテクスチャ
		// 共有のトゥーンについては「PmxEditor/_data/toon」に画像があったけど、とりあえずは無視
		else
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 4),
				white_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// pmxにはサブテクスチャのスフィアマップの情報もあるが、とりあえずは無視
	}

	// pmxxの頂点バッファのビュー
	D3D12_VERTEX_BUFFER_VIEW pmx_vertex_buffer_view{
		.BufferLocation = pmx_vertex_buffer_resource.first->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(model_vertex_data) * pmx_vertex.size()),
		.StrideInBytes = static_cast<UINT>(sizeof(model_vertex_data)),
	};

	// pmxのインデックスバッファのビュー
	D3D12_INDEX_BUFFER_VIEW pmx_index_buffer_view{
		.BufferLocation = pmx_index_buffer_resource.first->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(index) * pmx_surface.size()),
		.Format = DXGI_FORMAT_R32_UINT
	};


	// imgui用のディスクリプタヒープ
	// 大きさは1でいいっぽい?
	auto imgui_descriptor_heap = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);


	//
	// ルートシグネチャとグラフィクスパイプライン
	//

	// pmxのフレームバッファに描画する際のルートシグネチャ
	auto pmx_root_signature = dx12w::create_root_signature(device.get(), pmx_descriptor_table,
		{/*通常のテクスチャのサンプラ*/ {D3D12_FILTER_MIN_MAG_MIP_POINT ,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_TEXTURE_ADDRESS_MODE_WRAP,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_COMPARISON_FUNC_NEVER},
		/*トゥーン用サンプラ*/{D3D12_FILTER_MIN_MAG_MIP_POINT ,D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,D3D12_TEXTURE_ADDRESS_MODE_CLAMP,D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,D3D12_COMPARISON_FUNC_NEVER} });

	// pmxをフレームバッファに描画する際のグラフィクスパイプライン
	auto pmx_graphics_pipeline_state = dx12w::create_graphics_pipeline(device.get(), pmx_root_signature.get(),
		{ { "POSITION",DXGI_FORMAT_R32G32B32_FLOAT },{ "NORMAL",DXGI_FORMAT_R32G32B32_FLOAT },{ "TEXCOORD",DXGI_FORMAT_R32G32_FLOAT },
		{ "BONEINDEX",DXGI_FORMAT_R32G32B32A32_UINT },{ "BONEWEIGHT",DXGI_FORMAT_R32G32B32A32_FLOAT } },
		{ FRAME_BUFFER_FORMAT }, { {pmx_vertex_shader.data(),pmx_vertex_shader.size()},{pmx_index_shader.data(),pmx_index_shader.size()} },
		true, true, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);


	//
	// Imguiの設定
	//

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device.get(), 1,
		DXGI_FORMAT_R8G8B8A8_UNORM, imgui_descriptor_heap.get(),
		imgui_descriptor_heap.get_CPU_handle(),
		imgui_descriptor_heap.get_GPU_handle());

	//
	// bullet
	//

	// こいつがbullet_worldより先（後？）に消えるとエラーはくので注意
	// TODO: bullet_worldとの解放の順についてやる!
	std::vector<bullet_rigidbody> bullet_rigidbody(pmx_rigidbody.size());
	for (std::size_t i = 0; i < bullet_rigidbody.size(); i++) {
		bullet_rigidbody[i] = create_shape_bullet_rigidbody(pmx_rigidbody[i]);
	}

	std::vector<bullet_joint> bullet_joint(pmx_joint.size());
	for (std::size_t i = 0; i < bullet_joint.size(); i++) {
		bullet_joint[i] = create_bullet_joint(pmx_joint[i], bullet_rigidbody);
	}

	bullet_world bullet_world{};
	
	for (std::size_t i = 0; i < bullet_rigidbody.size(); i++) {
		bullet_world.dynamics_world.addRigidBody(bullet_rigidbody[i].rigidbody.get(), 1 << pmx_rigidbody[i].group, pmx_rigidbody[i].non_collision_group);
	}
	for (std::size_t i = 0; i < bullet_joint.size(); i++) {
		bullet_world.dynamics_world.addConstraint(bullet_joint[i].spring.get());
	}
	
	auto debug_box_resource = std::make_unique<shape_resource>(device.get(), "data/box.obj", camera_data_resource.first.get());
	auto debug_sphere_resoruce = std::make_unique<shape_resource>(device.get(), "data/sphere.obj", camera_data_resource.first.get());
	auto debug_capsule_resrouce = std::make_unique<shape_resource>(device.get(), "data/capsule.obj", camera_data_resource.first.get());

	auto debug_shape_pipeline = std::make_unique<shape_pipeline>(device.get(), FRAME_BUFFER_FORMAT);

	debug_draw debug_draw{};
	debug_draw.setDebugMode(btIDebugDraw::DBG_DrawWireframe
		| btIDebugDraw::DBG_DrawContactPoints
		| btIDebugDraw::DBG_DrawConstraints);

	// デバック用のインスタンス割当て
	bullet_world.dynamics_world.setDebugDrawer(&debug_draw);
	

	//
	// その他設定
	//

	D3D12_VIEWPORT viewport{
		.TopLeftX = 0.f,
		.TopLeftY = 0.f,
		.Width = static_cast<float>(WINDOW_WIDTH),
		.Height = static_cast<float>(WINDOW_HEIGHT),
		.MinDepth = 0.f,
		.MaxDepth = 1.f,
	};

	D3D12_RECT scissor_rect{
		.left = 0,
		.top = 0,
		.right = static_cast<LONG>(WINDOW_WIDTH),
		.bottom = static_cast<LONG>(WINDOW_HEIGHT),
	};

	XMFLOAT3 eye{ 0.f,14.f,-16.f };
	XMFLOAT3 target{ 0.f,10.f,0.f };
	XMFLOAT3 up{ 0,1,0 };
	float asspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
	float view_angle = XM_PIDIV2;
	float camera_near_z = 0.01f;
	float camera_far_z = 1000.f;

	XMFLOAT3 direction_light_color{ 0.4f,0.4f,0.4f };
	XMFLOAT3 direction_light_dir{ 0.5f,0.5f,-0.5f };

	float model_rotation_x = 0.f;
	float model_rotation_y = 0.f;
	float model_rotation_z = 0.f;

	// 子ボーンのインデックスを取得するための配列を取得
	auto const to_children_bone_index = get_to_children_bone_index(pmx_bone);

	// ボーンの名前から対応するボーンのインデックスを取得する際に使用する
	auto const bone_name_to_bone_index = get_bone_name_to_bone_index(pmx_bone);

	// ボーンの名前から対応するボーンのアニメーションを取得する際に使用する
	auto const bone_name_to_bone_motion_data = get_bone_name_to_bone_motion_data(vmd_frame_data);

	// モーフの名前かった対応するモーフのインデックスるを取得する際に使用する
	auto const morph_index_to_morph_vertex_index = get_morph_index_to_morph_vertex_index(pmx_vertex_morph);

	// モーフの名前から対応するモーフのアニメーションを取得する際に使用する
	auto const morph_name_to_morph_motion_data = get_morph_name_to_morph_motion_data(vmd_morph_data);

	direction_light_data direction_light{
		.dir = direction_light_dir,
		.color = direction_light_color,
	};

	int frame_num = 0;
	bool auto_animation = false;
	bool use_vpd = false;

	// IKの処理でボーンを回転させる数
	int ik_rotation_num = -1;
	// IKによって最後に回転させたボーンの各制限を無視した回転を表示
	bool check_ideal_rotation = false;
	// 残余回転を適用するかどうか
	bool is_residual = false;

	float offset_x = 0.f;
	float offset_y = 0.f;
	float offset_z = 0.f;

	auto prev_frame_time = std::chrono::system_clock::now();
	double elapsed = 0.0;

	// 物理演算用
	auto pysics_prev_time = std::chrono::system_clock::now();

	std::vector<bone_data> bone_data(MAX_BONE_NUM);

	int morph_index = -1;

	bool is_display_rigidbody = false;

	float head_rotation_x = 0.f;
	float head_rotation_y = 0.f;
	float head_rotation_z = 0.f;

	while (dx12w::update_window())
	{

		auto back_buffer_index = swap_chain->GetCurrentBackBufferIndex();

		command_manager.reset_list(0);

		//
		// データの更新
		//

		auto const view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto const proj = DirectX::XMMatrixPerspectiveFovLH(
			view_angle,
			asspect,
			camera_near_z,
			camera_far_z
		);

		// ボーンのデータをを初期化
		initialize_bone_data(bone_data);

		// 指定されているボーンに適当な行列を設定
		if (use_vpd)
			set_bone_data_from_vpd(bone_data, vpd_data, bone_name_to_bone_index);
		else
			set_bone_data_from_vmd(bone_data, bone_name_to_bone_motion_data, pmx_bone, bone_name_to_bone_index, frame_num);

		// IKデバック用
		{
			auto iter = bone_name_to_bone_index.find(L"右足ＩＫ");
			if (iter != bone_name_to_bone_index.end())
				bone_data[iter->second].transform += XMVECTOR{ offset_x, offset_y, offset_z };
		}

		// 物理演算デバッグ用
		{
			bone_data[15].rotation = XMQuaternionMultiply(bone_data[15].rotation, XMQuaternionRotationRollPitchYaw(head_rotation_x, head_rotation_y, head_rotation_z));
		}

		auto const root_index = bone_name_to_bone_index.contains(L"全ての親") ? bone_name_to_bone_index.at(L"全ての親") : 0;

		// それぞれの親のノードの回転、移動の行列を子へ伝播させる
		set_to_world_matrix(bone_data, to_children_bone_index, root_index, XMMatrixIdentity(), pmx_bone);

		// IK
		int ik_rotation_counter = 0;
		recursive_aplly_ik(bone_data, root_index, to_children_bone_index, pmx_bone, ik_rotation_num, &ik_rotation_counter, check_ideal_rotation);

		// 付与の解決
		recursive_aplly_grant(bone_data, root_index, to_children_bone_index, pmx_bone);

		for (std::size_t i = 0; i < pmx_rigidbody.size(); i++)
		{
			// ボーン追従
			if (pmx_rigidbody[i].rigidbody_type == 0)
			{
				auto bone_index = pmx_rigidbody[i].bone_index;

				// 対象ボーンのワールド座標への変換行列
				auto const to_world =
					XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[bone_index].position)) *
					XMMatrixRotationQuaternion(bone_data[bone_index].rotation) *
					XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[bone_index].position)) *
					XMMatrixTranslationFromVector(bone_data[bone_index].transform) *
					bone_data[bone_index].to_world;

				// 対象ボーンのワールド座標への変換の回転のみを表すクオータニオン
				auto const to_world_rot = XMQuaternionRotationMatrix(to_world);

				// ボーンのワールド座標
				auto const bone_world_pos = XMVector3Transform(XMLoadFloat3(&pmx_bone[bone_index].position), to_world);

				// ローカル座標でのボーンの座標から剛体の座標へのベクトル
				auto const bone_to_rigidbody_local_vec = XMVectorSubtract(XMLoadFloat3(&pmx_rigidbody[i].position), XMLoadFloat3(&pmx_bone[bone_index].position));

				// ワールド座標でのボーンの座標から剛体の座標へのベクトル
				auto const bone_to_rigidbody_world_vec = XMVector3Rotate(bone_to_rigidbody_local_vec, to_world_rot);

				// 回転の合計
				auto const rot_sum = XMQuaternionMultiply(
					XMQuaternionRotationRollPitchYaw(pmx_rigidbody[i].rotation.x, pmx_rigidbody[i].rotation.y, pmx_rigidbody[i].rotation.z)
					, to_world_rot);

				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(
					bone_world_pos.m128_f32[0] + bone_to_rigidbody_world_vec.m128_f32[0],
					bone_world_pos.m128_f32[1] + bone_to_rigidbody_world_vec.m128_f32[1],
					bone_world_pos.m128_f32[2] + bone_to_rigidbody_world_vec.m128_f32[2]));
				transform.setRotation(btQuaternion(rot_sum.m128_f32[0], rot_sum.m128_f32[1], rot_sum.m128_f32[2], rot_sum.m128_f32[3]));
				
				bullet_rigidbody[i].rigidbody->setWorldTransform(transform);
			}

			// 全ての剛体をアクティブにする
			bullet_rigidbody[i].rigidbody->activate(true);
		}

		//
		// 物理エンジンのシュミレーション
		//

		auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - pysics_prev_time).count();
		if (delta_time >= 1.f / 60.f * 100.f)
		{
			try {
				bullet_world.dynamics_world.stepSimulation(delta_time, 10);
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
			pysics_prev_time = std::chrono::system_clock::now();
		}

		//
		// 物理シュミレーションの結果を反映させる
		//

		// TODO:
		debug_draw.sphereData.clear();
		debug_draw.boxData.clear();
		debug_draw.capsuleData.clear();

		//
		// 物理エンジンの結果を描画するために準備
		//



		bullet_world.dynamics_world.debugDrawWorld();

		for (auto& s : debug_draw.sphereData)
			s.transform *= XMMatrixRotationRollPitchYaw(model_rotation_x, model_rotation_y, model_rotation_z);
		for (auto& s : debug_draw.boxData)
			s.transform *= XMMatrixRotationRollPitchYaw(model_rotation_x, model_rotation_y, model_rotation_z);
		for (auto& s : debug_draw.capsuleData)
			s.transform *= XMMatrixRotationRollPitchYaw(model_rotation_x, model_rotation_y, model_rotation_z);


		debug_sphere_resoruce->setShapeData(debug_draw.sphereData.begin(), debug_draw.sphereData.end());
		debug_box_resource->setShapeData(debug_draw.boxData.begin(), debug_draw.boxData.end());
		debug_capsule_resrouce->setShapeData(debug_draw.capsuleData.begin(), debug_draw.capsuleData.end());

		//
		// マップする
		//

		// 頂点
		{
			model_vertex_data* tmp = nullptr;
			pmx_vertex_buffer_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
			std::copy(pmx_vertex.begin(), pmx_vertex.end(), tmp);

			
			// imguiからモーフがせんたくされている場合
			if (0 <= morph_index && morph_index < pmx_vertex_morph.size())
			{
				for (auto const& m : pmx_vertex_morph[morph_index].data)
				{
					tmp[m.index].position.x += m.offset.x;
					tmp[m.index].position.y += m.offset.y;
					tmp[m.index].position.z += m.offset.z;
				}
			}
			else
			{
				// アニメーションからモーフをしていする場合
				for (auto const& morph : pmx_vertex_morph)
				{
					auto weight = get_morph_motion_weight(morph_name_to_morph_motion_data, morph.name, frame_num);
					for (auto const& m : morph.data)
					{
						tmp[m.index].position.x += m.offset.x * weight;
						tmp[m.index].position.y += m.offset.y * weight;
						tmp[m.index].position.z += m.offset.z * weight;
					}
				}
			}
			

			// グループモーフ
			for (auto const& morph : pmx_group_morph)
			{
				auto weight = get_morph_motion_weight(morph_name_to_morph_motion_data, morph.name, frame_num);

				// グループに含まれる頂点モーフを走査
				for (auto const& gm : morph.data)
				{
					auto iter = morph_index_to_morph_vertex_index.find(gm.index);
					if (iter != morph_index_to_morph_vertex_index.end())
					{
						// 頂点モーフに含まれる頂点を走査
						for (auto const& m : pmx_vertex_morph[iter->second].data)
						{
							tmp[m.index].position.x += m.offset.x * weight * gm.morph_factor;
							tmp[m.index].position.y += m.offset.y * weight * gm.morph_factor;
							tmp[m.index].position.z += m.offset.z * weight * gm.morph_factor;
						}
					}
				}
			}

			pmx_vertex_buffer_resource.first->Unmap(0, nullptr);
		}


		{
			model_data* tmp = nullptr;
			model_data_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));

			tmp->world = XMMatrixRotationX(model_rotation_x) * XMMatrixRotationY(model_rotation_y) * XMMatrixRotationZ(model_rotation_z);
			bone_data_to_bone_matrix(bone_data, tmp->bone, pmx_bone);


			//
			//
			//
			
			for (std::size_t i = 0; i < pmx_rigidbody.size(); i++)
			{
				// 物理演算によって移動するボーン
				if (pmx_rigidbody[i].rigidbody_type == 1)
				{
					auto bone_index = pmx_rigidbody[i].bone_index;
					auto const& transform = bullet_rigidbody[i].rigidbody->getWorldTransform();

					auto trans = XMVECTOR{
						static_cast<float>(transform.getOrigin().x()),
						static_cast<float>(transform.getOrigin().y()),
						static_cast<float>(transform.getOrigin().z()),
					};

					auto rot = XMVECTOR{
						static_cast<float>(transform.getRotation().x()),
						static_cast<float>(transform.getRotation().y()),
						static_cast<float>(transform.getRotation().z()),
						static_cast<float>(transform.getRotation().w())
					};

					auto parent_rot = XMQuaternionRotationMatrix(bone_data[bone_index].to_world);
					auto rigidbody_rot = XMQuaternionRotationRollPitchYaw(pmx_rigidbody[i].rotation.x, pmx_rigidbody[i].rotation.y, pmx_rigidbody[i].rotation.z);

					// ローカル座標での剛体からボーンへのベクトル
					auto const rigidbody_to_bone_local_vec = XMVectorSubtract(XMLoadFloat3(&pmx_bone[bone_index].position), XMLoadFloat3(&pmx_rigidbody[i].position));

					// 物理シュミレーションによって位置を修正された剛体からみた剛体からボーンへのベクトルを変換
					auto const rigidbody_to_bone_world_vec = XMVector3Rotate(rigidbody_to_bone_local_vec, XMQuaternionMultiply(XMQuaternionMultiply(rot, XMQuaternionInverse(parent_rot)), XMQuaternionInverse(rigidbody_rot)));

					// 修正後のワールド座標でのボーンの位置
					auto const new_world_bone_position = trans + rigidbody_to_bone_local_vec;

					tmp->bone[bone_index] = XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[bone_index].position)) *
						XMMatrixRotationQuaternion(XMQuaternionMultiply(rot, XMQuaternionInverse(rigidbody_rot))) *
						XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[bone_index].position)) *
						XMMatrixTranslationFromVector(XMVectorSubtract(new_world_bone_position, XMLoadFloat3(&pmx_bone[bone_index].position)));
				}
			}
		}

		{
			camera_data* tmp = nullptr;
			camera_data_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));

			tmp->view = view;
			tmp->viewInv = XMMatrixInverse(nullptr, view);
			tmp->proj = proj;
			tmp->projInv = XMMatrixInverse(nullptr, proj);
			tmp->viewProj = view * proj;
			tmp->viewProjInv = XMMatrixInverse(nullptr, tmp->viewProj);
			tmp->cameraNear = camera_near_z;
			tmp->cameraFar = camera_far_z;
			tmp->screenWidth = WINDOW_WIDTH;
			tmp->screenHeight = WINDOW_HEIGHT;
			tmp->eyePos = eye;
		}

		{
			direction_light_data* tmp = nullptr;
			direction_light_data_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
			*tmp = direction_light;
		}


		//
		// Imguiの準備
		//

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("conf");
		ImGui::SliderFloat("model rotaion x", &model_rotation_x, -XM_PI, XM_PI);
		ImGui::SliderFloat("model rotaion y", &model_rotation_y, -XM_PI, XM_PI);
		ImGui::SliderFloat("model rotaion z", &model_rotation_z, -XM_PI, XM_PI);
		ImGui::InputFloat3("eye", &eye.x);
		ImGui::InputFloat3("target", &target.x);

		ImGui::InputInt("frame num", &frame_num);
		ImGui::Checkbox("auto animation", &auto_animation);

		ImGui::InputInt("ik rotation num", &ik_rotation_num);
		ImGui::Checkbox("is ideal rotation", &check_ideal_rotation);
		ImGui::Checkbox("is residual", &is_residual);

		ImGui::SliderFloat("offset x", &offset_x, -10.f, 10.f);
		ImGui::SliderFloat("offset y", &offset_y, -10.f, 10.f);
		ImGui::SliderFloat("offset z", &offset_z, -10.f, 10.f);

		ImGui::Checkbox("use vpd", &use_vpd);

		ImGui::InputInt("morph index", &morph_index);

		ImGui::Checkbox("is display rigidbody", &is_display_rigidbody);

		ImGui::SliderFloat("head rotation x", &head_rotation_x, -XM_PI, XM_PI);
		ImGui::SliderFloat("head rotation y", &head_rotation_y, -XM_PI, XM_PI);
		ImGui::SliderFloat("head rotation z", &head_rotation_z, -XM_PI, XM_PI);

		ImGui::End();

		ImGui::Render();


		//
		// pmxの描画のコマンドを積む
		//

		command_manager.get_list()->RSSetViewports(1, &viewport);
		command_manager.get_list()->RSSetScissorRects(1, &scissor_rect);

		dx12w::resource_barrior(command_manager.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_RENDER_TARGET);
		dx12w::resource_barrior(command_manager.get_list(), depth_buffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		command_manager.get_list()->ClearRenderTargetView(frame_buffer_descriptor_heap_RTV.get_CPU_handle(back_buffer_index), frame_buffer_clear_value.Color, 0, nullptr);
		command_manager.get_list()->ClearDepthStencilView(depth_buffer_descriptor_heap_DSV.get_CPU_handle(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

		auto frame_buffer_cpu_handle = frame_buffer_descriptor_heap_RTV.get_CPU_handle(back_buffer_index);
		auto depth_buffer_cpu_handle = depth_buffer_descriptor_heap_DSV.get_CPU_handle(0);
		command_manager.get_list()->OMSetRenderTargets(1, &frame_buffer_cpu_handle, false, &depth_buffer_cpu_handle);

		command_manager.get_list()->SetGraphicsRootSignature(pmx_root_signature.get());
		{
			auto tmp = pmx_descriptor_heap_CBV_SRV_UAV.get();
			command_manager.get_list()->SetDescriptorHeaps(1, &tmp);
		}
		command_manager.get_list()->SetPipelineState(pmx_graphics_pipeline_state.get());
		command_manager.get_list()->IASetVertexBuffers(0, 1, &pmx_vertex_buffer_view);
		command_manager.get_list()->IASetIndexBuffer(&pmx_index_buffer_view);
		command_manager.get_list()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 共通のビューの設定
		command_manager.get_list()->SetGraphicsRootDescriptorTable(0, pmx_descriptor_heap_CBV_SRV_UAV.get_GPU_handle(0));

		UINT index_offset = 0;
		for (std::size_t i = 0; i < pmx_material.size(); i++)
		{
			// マテリアルごとのビューの設定
			// 共通のビューについてはすでに設定してあるので必要なし
			command_manager.get_list()->SetGraphicsRootDescriptorTable(1, pmx_descriptor_heap_CBV_SRV_UAV.get_GPU_handle(pmx_common_view_num + pmx_material_view_num * i));

			// マテリアルの頂点のオフセットを指定し描画
			command_manager.get_list()->DrawIndexedInstanced(pmx_material_2[i].vertex_number, 1, index_offset, 0, 0);

			// オフセットの更新
			index_offset += pmx_material_2[i].vertex_number;
		}

		//
		// 剛体のDebug用の描画
		//

		if (is_display_rigidbody)
		{
			command_manager.get_list()->RSSetViewports(1, &viewport);
			command_manager.get_list()->RSSetScissorRects(1, &scissor_rect);

			// デプスバッファはいったんクリアしておく
			command_manager.get_list()->ClearDepthStencilView(depth_buffer_descriptor_heap_DSV.get_CPU_handle(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

			command_manager.get_list()->OMSetRenderTargets(1, &frame_buffer_cpu_handle, false, &depth_buffer_cpu_handle);

			debug_shape_pipeline->draw(command_manager.get_list(), *debug_box_resource);
			debug_shape_pipeline->draw(command_manager.get_list(), *debug_sphere_resoruce);
			debug_shape_pipeline->draw(command_manager.get_list(), *debug_capsule_resrouce);
		}


		//
		// Imguiの描画のコマンドを積む
		//

		auto imgui_descriptor_heap_ptr = imgui_descriptor_heap.get();
		command_manager.get_list()->SetDescriptorHeaps(1, &imgui_descriptor_heap_ptr);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_manager.get_list());


		dx12w::resource_barrior(command_manager.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_COMMON);
		dx12w::resource_barrior(command_manager.get_list(), depth_buffer, D3D12_RESOURCE_STATE_COMMON);


		//
		// コマンド発行
		//

		command_manager.get_list()->Close();
		command_manager.excute();
		command_manager.signal();

		command_manager.wait(0);

		swap_chain->Present(1, 0);


		//
		// その他
		//

		if (auto_animation)
		{
			auto current_time = std::chrono::system_clock::now();
			elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(current_time - prev_frame_time).count();

			// 30fps
			if (elapsed >= 1000.0 / 30.0) {
				frame_num++;
				elapsed -= 1000.0 / 30.0;
			}
		}

		prev_frame_time = std::chrono::system_clock::now();
	}


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}