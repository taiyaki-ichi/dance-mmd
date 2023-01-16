#pragma once
#include<Windows.h>
#include<dxgi.h>
#include<cstddef>

constexpr LONG WINDOW_WIDTH = 1024;
constexpr LONG WINDOW_HEIGHT = 768;

constexpr std::size_t COMMAND_ALLOCATOR_NUM = 1;

constexpr DXGI_FORMAT FRAME_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr std::size_t FRAME_BUFFER_NUM = 2;

constexpr DXGI_FORMAT DEPTH_BUFFER_FORMAT = DXGI_FORMAT_D32_FLOAT;

constexpr DXGI_FORMAT PMX_TEXTURE_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

constexpr std::size_t MAX_BONE_NUM = 516;