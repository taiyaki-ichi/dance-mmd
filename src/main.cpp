#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"../external/imgui/imgui.h"
#include"../external/imgui/imgui_impl_dx12.h"
#include"../external/imgui/imgui_impl_win32.h"
#include"../external/mmd-loader/mmdl/pmx_loader.hpp"
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include"../external/stb/stb_image.h"
#include<fstream>
#include<algorithm>
#include<DirectXMath.h>



using namespace DirectX;

constexpr LONG WINDOW_WIDTH = 1024;
constexpr LONG WINDOW_HEIGHT = 768;

constexpr std::size_t COMMAND_ALLOCATOR_NUM = 1;

constexpr DXGI_FORMAT FRAME_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr std::size_t FRAME_BUFFER_NUM = 2;

constexpr DXGI_FORMAT DEPTH_BUFFER_FORMAT = DXGI_FORMAT_D32_FLOAT;

constexpr DXGI_FORMAT PMX_TEXTURE_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

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

struct vertex
{
	XMFLOAT3 position{};
	XMFLOAT3 normal{};
	XMFLOAT2 uv{};
};

using index = std::uint32_t;

struct model_data
{
	XMMATRIX world;
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

struct material_data
{
	XMFLOAT4 diffuse;
	XMFLOAT3 specular;
	float specularity;
	XMFLOAT3 ambient;
	float _pad0;
};


int main()
{
	auto hwnd = dx12w::create_window(L"dance-mmd", WINDOW_WIDTH, WINDOW_HEIGHT, wnd_proc);

	auto device = dx12w::create_device();
	auto command_manager = dx12w::create_command_manager<COMMAND_ALLOCATOR_NUM>(device.get());
	auto swap_chain = dx12w::create_swap_chain(command_manager.get_queue(), hwnd, FRAME_BUFFER_FORMAT, FRAME_BUFFER_NUM);


	// TODO: 
	// const wchar_t* file_path = L"../../../3dmodel/パイモン/派蒙.pmx";
	const wchar_t* file_path = L"../../../3dmodel/kizunaai/kizunaai.pmx";

	std::ifstream file{ file_path ,std::ios::binary };
	auto pmx_header = mmdl::load_header<>(file);
	auto pmx_info = mmdl::load_info<std::wstring>(file, pmx_header.encode);
	auto pmx_vertex = mmdl::load_vertex<std::vector, XMFLOAT2, XMFLOAT3, XMFLOAT4>(file, pmx_header.add_uv_number, pmx_header.bone_index_size);
	auto pmx_surface = mmdl::load_surface<std::vector>(file, pmx_header.vertex_index_size);
	auto pmx_texture_path = mmdl::load_texture_path<std::vector, std::wstring>(file, pmx_header.encode);
	auto pmx_material = mmdl::load_material<std::vector, std::wstring, XMFLOAT3, XMFLOAT4>(file, pmx_header.encode, pmx_header.texture_index_size);


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

	std::vector<dx12w::resource_and_state> pmx_texture_resrouce{};
	{
		// TODO: 
		// const wchar_t* directory_path = L"../../../3dmodel/パイモン/";
		const wchar_t* directory_path = L"../../../3dmodel/kizunaai/";

		for (auto& path : pmx_texture_path)
		{
			char buff[256];
			stbi_convert_wchar_to_utf8(buff, 256, (std::wstring{ directory_path } + path).data());
			int x, y, n;
			std::uint8_t* data = stbi_load(buff, &x, &y, &n, 0);

			auto dst_texture_resource = dx12w::create_commited_texture_resource(device.get(),
				PMX_TEXTURE_FORMAT, x, y, 2, 1, 1, D3D12_RESOURCE_FLAG_NONE);

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
	}

	std::vector<dx12w::resource_and_state> material_resource{};
	{
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
	}

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
	// テクスチャ、material_data
	constexpr UINT material_view_num = 2;
	auto pmx_descriptor_heap_CBV_SRV_UAV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3 + material_view_num * pmx_material.size());
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(0), model_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(model_data), 256));
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(1), camera_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(camera_data), 256));
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(2), direction_light_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(direction_light_data), 256));
	for (std::size_t i = 0; i < pmx_material.size(); i++)
	{
		dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 0),
			pmx_texture_resrouce[pmx_material[i].texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + material_view_num * i + 1),
			material_resource[i].first.get(), dx12w::alignment<UINT64>(sizeof(material_data), 256));
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
	//
	//

	auto pmx_root_signature = dx12w::create_root_signature(device.get(),
		{ {{/*model_data, camera_data, direction_data*/D3D12_DESCRIPTOR_RANGE_TYPE_CBV,3}},{{/*texture*/D3D12_DESCRIPTOR_RANGE_TYPE_SRV},{/*material_data*/D3D12_DESCRIPTOR_RANGE_TYPE_CBV}} },
		{ {D3D12_FILTER_MIN_MAG_MIP_POINT ,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_TEXTURE_ADDRESS_MODE_WRAP,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_COMPARISON_FUNC_NEVER} });

	auto pmx_graphics_pipeline_state = dx12w::create_graphics_pipeline(device.get(), pmx_root_signature.get(),
		{ { "POSITION",DXGI_FORMAT_R32G32B32_FLOAT },{ "NORMAL",DXGI_FORMAT_R32G32B32_FLOAT },{ "TEXCOORD",DXGI_FORMAT_R32G32_FLOAT } },
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

	XMFLOAT3 eye{ 0.f,5.f,-6.f };
	XMFLOAT3 target{ 0.f,5.f,0.f };
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


	camera_data camera{};

	direction_light_data direction_light{};
	direction_light.color = direction_light_color;
	direction_light.dir = direction_light_dir;


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
	}


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}