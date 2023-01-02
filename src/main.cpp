#include"../external/directx12-wrapper/dx12w/dx12w.hpp"


int main()
{
	constexpr LONG widnow_width = 1024;
	constexpr LONG window_height = 768;

	auto window = dx12w::create_window(L"dance-mmd", widnow_width, window_height);

	while (dx12w::update_window())
	{

	}

	return 0;
}