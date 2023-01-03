#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"../external/imgui/imgui.h"
#include"../external/imgui/imgui_impl_dx12.h"
#include"../external/imgui/imgui_impl_win32.h"
#include"../external/mmd-loader/mmdl/pmx_loader.hpp"
#include<fstream>
#include<DirectXMath.h>

constexpr LONG WINDOW_WIDTH = 1024;
constexpr LONG WINDOW_HEIGHT = 768;

constexpr std::size_t COMMAND_ALLOCATOR_NUM = 1;

constexpr DXGI_FORMAT FRAME_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr std::size_t FRAME_BUFFER_NUM = 2;

constexpr DXGI_FORMAT DEPTH_BUFFER_FORMAT = DXGI_FORMAT_D32_FLOAT;

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
	auto command_manamger = dx12w::create_command_manager<COMMAND_ALLOCATOR_NUM>(device.get());
	auto swap_chain = dx12w::create_swap_chain(command_manamger.get_queue(), hwnd, FRAME_BUFFER_FORMAT, FRAME_BUFFER_NUM);

	{
		const wchar_t* file_path = L"../../../3dmodel/パイモン/派蒙.pmx";
		std::ifstream file{ file_path ,std::ios::binary };
		auto header = mmdl::load_header<>(file);
		auto info = mmdl::load_info<std::wstring>(file, header.encode);
		auto vertex = mmdl::load_vertex<std::vector, DirectX::XMFLOAT2, DirectX::XMFLOAT3, DirectX::XMFLOAT4>(file, header.add_uv_number, header.bone_index_size);
		auto surface = mmdl::load_surface<std::vector>(file, header.vertex_index_size);
		auto texture_path = mmdl::load_texture_path<std::vector, std::wstring>(file, header.encode);
		auto material = mmdl::load_material<std::vector, std::wstring, DirectX::XMFLOAT3, DirectX::XMFLOAT4>(file, header.encode, header.texture_index_size);
	}

	D3D12_CLEAR_VALUE frame_buffer_clear_value{
		.Format = FRAME_BUFFER_FORMAT,
		.Color = {0.5f,0.5f,0.5f,1.f},
	};

	D3D12_CLEAR_VALUE depth_clear_value{
		.Format = DEPTH_BUFFER_FORMAT,
		.DepthStencil{.Depth = 1.f}
	};

	std::array<dx12w::resource_and_state, FRAME_BUFFER_NUM> frame_buffer_resource{};
	for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
	{
		ID3D12Resource* tmp = nullptr;
		swap_chain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&tmp));
		frame_buffer_resource[i] = std::make_pair(dx12w::release_unique_ptr<ID3D12Resource>{tmp}, D3D12_RESOURCE_STATE_COMMON);
	}

	auto depth_buffer = dx12w::create_commited_texture_resource(device.get(), DEPTH_BUFFER_FORMAT, WINDOW_WIDTH, WINDOW_HEIGHT,
		2, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depth_clear_value);

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

	dx12w::descriptor_heap frame_buffer_descriptor_heap_RTV{};
	{
		frame_buffer_descriptor_heap_RTV.initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_NUM);

		for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
			dx12w::create_texture2D_RTV(device.get(), frame_buffer_descriptor_heap_RTV.get_CPU_handle(i), frame_buffer_resource[i].first.get(), FRAME_BUFFER_FORMAT, 0, 0);
	}

	auto depth_buffer_descriptor_heap_DSV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	dx12w::create_texture2D_DSV(device.get(), depth_buffer_descriptor_heap_DSV.get_CPU_handle(0), depth_buffer.first.get(), DEPTH_BUFFER_FORMAT, 0);

	// imgui用のディスクリプタヒープ
	dx12w::descriptor_heap imgui_descriptor_heap{};
	imgui_descriptor_heap.initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

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


	while (dx12w::update_window())
	{
		auto back_buffer_index = swap_chain->GetCurrentBackBufferIndex();

		command_manamger.reset_list(0);

		//
		// Imguiの準備
		//

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		bool hoge = true;
		ImGui::ShowDemoWindow(&hoge);

		ImGui::Begin("Hello, world!");
		ImGui::Text("This is some useful text.");
		ImGui::End();

		// Rendering
		ImGui::Render();

		//
		//
		//

		command_manamger.get_list()->RSSetViewports(1, &viewport);
		command_manamger.get_list()->RSSetScissorRects(1, &scissor_rect);

		dx12w::resource_barrior(command_manamger.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_RENDER_TARGET);
		command_manamger.get_list()->ClearRenderTargetView(frame_buffer_descriptor_heap_RTV.get_CPU_handle(back_buffer_index), frame_buffer_clear_value.Color, 0, nullptr);

		auto frame_buffer_cpu_handle = frame_buffer_descriptor_heap_RTV.get_CPU_handle(back_buffer_index);
		auto depth_buffer_cpu_handle = depth_buffer_descriptor_heap_DSV.get_CPU_handle(0);
		command_manamger.get_list()->OMSetRenderTargets(1, &frame_buffer_cpu_handle, false, &depth_buffer_cpu_handle);

		//
		// フレームへの描画
		//

		// Imguiの描画
		auto imgui_descriptor_heap_ptr = imgui_descriptor_heap.get();
		command_manamger.get_list()->SetDescriptorHeaps(1, &imgui_descriptor_heap_ptr);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_manamger.get_list());


		dx12w::resource_barrior(command_manamger.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_COMMON);

		command_manamger.get_list()->Close();
		command_manamger.excute();
		command_manamger.signal();

		command_manamger.wait(0);

		swap_chain->Present(1, 0);
	}


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}