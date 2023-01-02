#include"../external/directx12-wrapper/dx12w/dx12w.hpp"


constexpr LONG WINDOW_WIDTH = 1024;
constexpr LONG WINDOW_HEIGHT = 768;

constexpr std::size_t COMMAND_ALLOCATOR_NUM = 1;

constexpr DXGI_FORMAT FRAME_BUFFER_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
constexpr std::size_t FRAME_BUFFER_NUM = 2;

constexpr DXGI_FORMAT DEPTH_BUFFER_FORMAT = DXGI_FORMAT_D32_FLOAT;

int main()
{
	auto hwnd = dx12w::create_window(L"dance-mmd", WINDOW_WIDTH, WINDOW_HEIGHT);

	auto device = dx12w::create_device();
	auto command_manamger = dx12w::create_command_manager<COMMAND_ALLOCATOR_NUM>(device.get());
	auto swap_chain = dx12w::create_swap_chain(command_manamger.get_queue(), hwnd, FRAME_BUFFER_FORMAT, FRAME_BUFFER_NUM);

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



	while (dx12w::update_window())
	{
		auto back_buffer_index = swap_chain->GetCurrentBackBufferIndex();

		command_manamger.reset_list(0);

		command_manamger.get_list()->RSSetViewports(1, &viewport);
		command_manamger.get_list()->RSSetScissorRects(1, &scissor_rect);

		dx12w::resource_barrior(command_manamger.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_RENDER_TARGET);
		command_manamger.get_list()->ClearRenderTargetView(frame_buffer_descriptor_heap_RTV.get_CPU_handle(back_buffer_index), frame_buffer_clear_value.Color, 0, nullptr);

		//
		//
		//

		dx12w::resource_barrior(command_manamger.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_COMMON);

		command_manamger.get_list()->Close();
		command_manamger.excute();
		command_manamger.signal();

		command_manamger.wait(0);

		swap_chain->Present(1, 0);
	}

	return 0;
}