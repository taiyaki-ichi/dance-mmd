#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"../external/imgui/imgui.h"
#include"../external/imgui/imgui_impl_dx12.h"
#include"../external/imgui/imgui_impl_win32.h"
#include"../external/mmd-loader/mmdl/pmx_loader.hpp"
#include"../external/mmd-loader/mmdl/vpd_loader.hpp"
#include"../external/mmd-loader/mmdl/vmd_loader.hpp"
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include"../external/stb/stb_image.h"
#include"function.hpp"
#include<fstream>
#include<algorithm>
#include<numeric>
#include<unordered_map>
#include<DirectXMath.h>


using namespace DirectX;

// extern宣言
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


	// TODO: 
	//const wchar_t* file_path = L"E:素材/原神/パイモン/派蒙.pmx";
	//const wchar_t* directory_path = L"E:素材/原神/パイモン/";
	//const wchar_t* file_path = L"E:素材/キズナアイ/KizunaAI_ver1.01/kizunaai/kizunaai.pmx";
	//const wchar_t* directory_path = L"E:素材/キズナアイ/KizunaAI_ver1.01/kizunaai/";
	const wchar_t* file_path = L"E:素材/ホロライブ/ときのそら公式mmd_ver2.1/ときのそら.pmx";
	const wchar_t* directory_path = L"E:素材/ホロライブ/ときのそら公式mmd_ver2.1/";
	//const wchar_t* file_path = L"E:素材/ホロライブ/Laplus_220516_1/Laplus/PMX/Laplus_220516_1.pmx";
	//const wchar_t* directory_path = L"E:素材/ホロライブ/Laplus_220516_1/Laplus/sourceimages/";


	std::ifstream file{ file_path ,std::ios::binary };
	auto pmx_header = mmdl::load_header<>(file);
	auto pmx_info = mmdl::load_info<std::wstring>(file, pmx_header.encode);
	auto pmx_vertex = mmdl::load_vertex<std::vector, XMFLOAT2, XMFLOAT3, XMFLOAT4>(file, pmx_header.add_uv_number, pmx_header.bone_index_size);
	auto pmx_surface = mmdl::load_surface<std::vector>(file, pmx_header.vertex_index_size);
	auto pmx_texture_path = mmdl::load_texture_path<std::vector, std::wstring>(file, pmx_header.encode);
	auto pmx_material = mmdl::load_material<std::vector, std::wstring, XMFLOAT3, XMFLOAT4>(file, pmx_header.encode, pmx_header.texture_index_size);
	auto pmx_bone = mmdl::load_bone<std::vector, std::wstring, XMFLOAT3, std::vector>(file, pmx_header.encode, pmx_header.bone_index_size);


	//const wchar_t* pose_file_path = L"../../../3dmodel/ポーズ25/4.vpd";
	const wchar_t* pose_file_path = L"../../../3dmodel/Pose Pack 6 - Snorlaxin/9.vpd";
	std::ifstream pose_file{ pose_file_path };

	// ポーズのデータ
	auto vpd_data = get_utf16_vpd_data(pose_file);

	// ボーンの名前から対応するボーンのインデックスを取得する際に使用する
	auto bone_name_to_bone_index = get_bone_name_to_bone_index(pmx_bone);

	const wchar_t* motion_file_path = L"../../../3dmodel/スイマジ/sweetmagic-right.vmd";
	std::ifstream motion_file{ motion_file_path,std::ios::binary };

	auto vmd_header = mmdl::load_vmd_header(motion_file);
	auto vmd_frame_data = mmdl::load_vmd_frame_data<std::vector, XMFLOAT3, XMFLOAT4>(motion_file, vmd_header.frame_data_num);

	D3D12_CLEAR_VALUE frame_buffer_clear_value{
	.Format = FRAME_BUFFER_FORMAT,
		.Color = { 0.5f,0.5f,0.5f,1.f },
	};

	D3D12_CLEAR_VALUE depth_clear_value{
		.Format = DEPTH_BUFFER_FORMAT,
		.DepthStencil{.Depth = 1.f}
	};

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


	//
	// リソース
	//

	std::array<dx12w::resource_and_state, FRAME_BUFFER_NUM> frame_buffer_resource{};
	for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
	{
		ID3D12Resource* tmp = nullptr;
		swap_chain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&tmp));
		frame_buffer_resource[i] = std::make_pair(dx12w::release_unique_ptr<ID3D12Resource>{tmp}, D3D12_RESOURCE_STATE_COMMON);
	}

	auto depth_buffer = dx12w::create_commited_texture_resource(device.get(), DEPTH_BUFFER_FORMAT, WINDOW_WIDTH, WINDOW_HEIGHT,
		2, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depth_clear_value);

	auto pmx_vertex_buffer_resource = dx12w::create_commited_upload_buffer_resource(device.get(), sizeof(vertex) * pmx_vertex.size());
	{
		vertex* tmp = nullptr;
		pmx_vertex_buffer_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		for (std::size_t i = 0; i < pmx_vertex.size(); i++)
		{
			tmp[i].position = pmx_vertex[i].position;
			tmp[i].normal = pmx_vertex[i].normal;
			tmp[i].uv = pmx_vertex[i].uv;
			for (std::size_t j = 0; j < 4; j++)
				tmp[i].bone_index[j] = pmx_vertex[i].bone[j];
			auto weight_sum = std::accumulate(pmx_vertex[i].weight.begin(), pmx_vertex[i].weight.end(), 0.f);
			for (std::size_t j = 0; j < 4; j++)
				tmp[i].bone_weight[j] = pmx_vertex[i].weight[j] / weight_sum;
		}
		pmx_vertex_buffer_resource.first->Unmap(0, nullptr);
	}

	auto pmx_index_buffer_resource = dx12w::create_commited_upload_buffer_resource(device.get(), sizeof(index) * pmx_surface.size());
	{
		index* tmp = nullptr;
		pmx_index_buffer_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		for (std::size_t i = 0; i < pmx_surface.size(); i++)
		{
			tmp[i] = pmx_surface[i];
		}
		pmx_index_buffer_resource.first->Unmap(0, nullptr);
	}

	auto model_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(model_data), 256));

	auto camera_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(camera_data), 256));

	auto direction_light_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(direction_light_data), 256));

	std::ifstream pmx_vertex_shader_cso{ L"shader/VertexShader.cso",std::ios::binary };
	auto pmx_vertex_shader = dx12w::load_blob(pmx_vertex_shader_cso);
	pmx_vertex_shader_cso.close();

	std::ifstream pmx_index_shader_cso{ L"shader/PixelShader.cso",std::ios::binary };
	auto pmx_index_shader = dx12w::load_blob(pmx_index_shader_cso);
	pmx_index_shader_cso.close();

	// 4x4の白いテクスチャ
	auto white_texture_resource = get_fill_4x4_texture_resource(device, command_manager, PMX_TEXTURE_FORMAT, 255);

	// 4x4の黒いテクスチャ
	auto black_texture_resource = get_fill_4x4_texture_resource(device, command_manager, PMX_TEXTURE_FORMAT, 0);

	// テクスチャのリソース
	auto pmx_texture_resrouce = get_pmx_texture_resrouce(device, command_manager, pmx_texture_path, PMX_TEXTURE_FORMAT, directory_path);

	// マテリアルのリソース
	auto pmx_material_resource = get_pmx_material_resource(device, pmx_material);

	//
	// ディスクリプタヒープとビュー
	//

	dx12w::descriptor_heap frame_buffer_descriptor_heap_RTV{};
	{
		frame_buffer_descriptor_heap_RTV.initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_NUM);

		for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
			dx12w::create_texture2D_RTV(device.get(), frame_buffer_descriptor_heap_RTV.get_CPU_handle(i), frame_buffer_resource[i].first.get(), FRAME_BUFFER_FORMAT, 0, 0);
	}

	auto depth_buffer_descriptor_heap_DSV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	dx12w::create_texture2D_DSV(device.get(), depth_buffer_descriptor_heap_DSV.get_CPU_handle(0), depth_buffer.first.get(), DEPTH_BUFFER_FORMAT, 0);

	// マテリアルごとのビューの数
	// テクスチャ、material_data、乗算スフィアマップ、加算スフィアマップ、トゥーン
	constexpr UINT material_view_num = 5;
	auto pmx_descriptor_heap_CBV_SRV_UAV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3 + material_view_num * pmx_material.size());
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(0), model_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(model_data), 256));
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(1), camera_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(camera_data), 256));
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(2), direction_light_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(direction_light_data), 256));
	for (std::size_t i = 0; i < pmx_material.size(); i++)
	{
		dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 0),
			pmx_texture_resrouce[pmx_material[i].texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 1),
			pmx_material_resource[i].first.get(), dx12w::alignment<UINT64>(sizeof(material_data), 256));

		// 乗算スフィアマップを使用する場合
		if (pmx_material[i].sphere_mode_value == mmdl::sphere_mode::sph)
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 2),
				pmx_texture_resrouce[pmx_material[i].sphere_texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// しない場合は白色のテクスチャ
		else
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 2),
				white_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// 加算スフィアマップを使用する場合
		if (pmx_material[i].sphere_mode_value == mmdl::sphere_mode::spa)
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 3),
				pmx_texture_resrouce[pmx_material[i].sphere_texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// しない場合は黒色のテクスチャ
		else {
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 3),
				black_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// サブテクスチャのスフィアマップはとりあえず無視

		// 非共有のトゥーン
		// インデックスにmaxが入っている場合があるのではじく
		if (pmx_material[i].toon_type_value == mmdl::toon_type::unshared && pmx_material[i].toon_texture < pmx_texture_path.size())
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 4),
				pmx_texture_resrouce[pmx_material[i].toon_texture].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// それ以外は白色のテクスチャ
		// 共有のトゥーンについては「PmxEditor/_data/toon」に画像があったけど、とりあえずは無視
		else
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 4),
				white_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
	}

	D3D12_VERTEX_BUFFER_VIEW pmx_vertex_buffer_view{
		.BufferLocation = pmx_vertex_buffer_resource.first->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(vertex) * pmx_vertex.size()),
		.StrideInBytes = static_cast<UINT>(sizeof(vertex)),
	};

	D3D12_INDEX_BUFFER_VIEW pmx_index_buffer_view{
		.BufferLocation = pmx_index_buffer_resource.first->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(index) * pmx_surface.size()),
		.Format = DXGI_FORMAT_R32_UINT
	};


	// imgui用のディスクリプタヒープ
	dx12w::descriptor_heap imgui_descriptor_heap{};
	imgui_descriptor_heap.initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);


	//
	// ルートシグネチャとグラフィクスパイプライン
	//

	auto pmx_root_signature = dx12w::create_root_signature(device.get(),
		{ {{/*model_data, camera_data, direction_data*/D3D12_DESCRIPTOR_RANGE_TYPE_CBV,3}},
		{{/*texture*/D3D12_DESCRIPTOR_RANGE_TYPE_SRV},{/*material_data*/D3D12_DESCRIPTOR_RANGE_TYPE_CBV},{/*乗算スフィアマップ、加算スフィアマップ、トゥーン*/D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3}} },
		{ {D3D12_FILTER_MIN_MAG_MIP_POINT ,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_TEXTURE_ADDRESS_MODE_WRAP,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_COMPARISON_FUNC_NEVER},
		/*トゥーン用サンプラ*/{D3D12_FILTER_MIN_MAG_MIP_POINT ,D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,D3D12_TEXTURE_ADDRESS_MODE_CLAMP,D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,D3D12_COMPARISON_FUNC_NEVER} });

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
	// その他設定
	//

	XMFLOAT3 eye{ 0.f,8.f,-10.f };
	XMFLOAT3 target{ 0.f,8.f,0.f };
	XMFLOAT3 up{ 0,1,0 };
	float asspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
	float view_angle = XM_PIDIV2;
	float camera_near_z = 0.01f;
	float camera_far_z = 1000.f;

	XMFLOAT3 direction_light_color{ 0.4f,0.4f,0.4f };
	XMFLOAT3 direction_light_dir{ 0.5f,0.5f,-0.5f };

	model_data model{};
	model.world = XMMatrixIdentity();
	float model_rotation_x = 0.f;
	float model_rotation_y = 0.f;
	float model_rotation_z = 0.f;
	std::fill(std::begin(model.bone), std::end(model.bone), XMMatrixIdentity());

	// 子ボーンのインデックスを取得するための配列を取得
	auto to_children_bone_index = get_to_children_bone_index(pmx_bone);

	auto bone_name_to_bone_motion_data = get_bone_name_to_bone_motion_data(vmd_frame_data);

	camera_data camera{};

	direction_light_data direction_light{};
	direction_light.color = direction_light_color;
	direction_light.dir = direction_light_dir;

	int frame_num = 0;

	float offset_x = 0.f;
	float offset_y = 0.f;
	float offset_z = 0.f;

	while (dx12w::update_window())
	{
		auto back_buffer_index = swap_chain->GetCurrentBackBufferIndex();

		command_manager.reset_list(0);

		auto view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

		auto proj = DirectX::XMMatrixPerspectiveFovLH(
			view_angle,
			asspect,
			camera_near_z,
			camera_far_z
		);

		model.world = XMMatrixRotationX(model_rotation_x) * XMMatrixRotationY(model_rotation_y) * XMMatrixRotationZ(model_rotation_z);

		camera.view = view;
		camera.viewInv = XMMatrixInverse(nullptr, camera.view);
		camera.proj = proj;
		camera.projInv = XMMatrixInverse(nullptr, camera.proj);
		camera.viewProj = view * proj;
		camera.viewProjInv = XMMatrixInverse(nullptr, camera.viewProj);
		camera.cameraNear = camera_near_z;
		camera.cameraFar = camera_far_z;
		camera.screenWidth = WINDOW_WIDTH;
		camera.screenHeight = WINDOW_HEIGHT;
		camera.eyePos = eye;

		// ボーンを初期化
		std::fill(std::begin(model.bone), std::end(model.bone), XMMatrixIdentity());

		// 指定されているボーンに適当な行列を設定
		// set_bone_matrix_from_vmd(model.bone, bone_name_to_bone_motion_data, pmx_bone, bone_name_to_bone_index, frame_num);
		set_bone_matrix_from_vpd(model.bone, vpd_data, pmx_bone, bone_name_to_bone_index);
		recursive_aplly_parent_matrix(model.bone, bone_name_to_bone_index[L"全ての親"], XMMatrixIdentity(), to_children_bone_index);

		// IK
		{
			auto pos = XMVector3Transform(XMLoadFloat3(&pmx_bone[bone_name_to_bone_index[L"右足首"]].position), XMMatrixTranslation(offset_x, offset_y, offset_z));
			XMFLOAT3 float3;
			XMStoreFloat3(&float3, pos);
			solve_CCDIK(model.bone, bone_name_to_bone_index[L"右足ＩＫ"], pmx_bone, float3, to_children_bone_index);
		}

		// それぞれの親のノードの回転、移動の行列を子へ伝播させる
		//recursive_aplly_parent_matrix(model.bone, bone_name_to_bone_index[L"全ての親"], XMMatrixIdentity(), to_children_bone_index);


		{
			model_data* tmp = nullptr;
			model_data_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
			*tmp = model;
		}

		{
			camera_data* tmp = nullptr;
			camera_data_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
			*tmp = camera;
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

		ImGui::SliderFloat("offset x", &offset_x, -10.f, 10.f);
		ImGui::SliderFloat("offset y", &offset_y, -10.f, 10.f);
		ImGui::SliderFloat("offset z", &offset_z, -10.f, 10.f);

		ImGui::End();

		// Rendering
		ImGui::Render();

		//
		//
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
		command_manager.get_list()->SetGraphicsRootDescriptorTable(0, pmx_descriptor_heap_CBV_SRV_UAV.get_GPU_handle(0));
		command_manager.get_list()->SetPipelineState(pmx_graphics_pipeline_state.get());
		command_manager.get_list()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		command_manager.get_list()->IASetVertexBuffers(0, 1, &pmx_vertex_buffer_view);
		command_manager.get_list()->IASetIndexBuffer(&pmx_index_buffer_view);

		UINT index_offset = 0;
		for (std::size_t i = 0; i < pmx_material.size(); i++)
		{
			command_manager.get_list()->SetGraphicsRootDescriptorTable(1, pmx_descriptor_heap_CBV_SRV_UAV.get_GPU_handle(3 + material_view_num * i));
			command_manager.get_list()->DrawIndexedInstanced(pmx_material[i].vertex_number, 1, index_offset, 0, 0);
			index_offset += pmx_material[i].vertex_number;
		}



		// Imguiの描画
		auto imgui_descriptor_heap_ptr = imgui_descriptor_heap.get();
		command_manager.get_list()->SetDescriptorHeaps(1, &imgui_descriptor_heap_ptr);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_manager.get_list());


		dx12w::resource_barrior(command_manager.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_COMMON);
		dx12w::resource_barrior(command_manager.get_list(), depth_buffer, D3D12_RESOURCE_STATE_COMMON);

		command_manager.get_list()->Close();
		command_manager.excute();
		command_manager.signal();

		command_manager.wait(0);

		swap_chain->Present(1, 0);

		// frame_num++;
	}


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}