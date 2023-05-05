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
	// Imgui�p�̏���
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
	// pmx�̓ǂݍ���
	//

	// file_name��pmx�t�@�C���ւ̃p�X
	// directory_path�̓t�@�C���̃t�H���_�ւ̃p�X
	//wchar_t const* file_path = L"E:�f��/���_/�p�C����/�h��.pmx";
	//wchar_t const* directory_path = L"E:�f��/���_/�p�C����/";
	wchar_t const* file_path = L"E:�f��/���_/�A���o�[/����.pmx";
	wchar_t const* directory_path = L"E:�f��/���_/�A���o�[/";
	//wchar_t const* file_path = L"E:�f��/�L�Y�i�A�C/KizunaAI_ver1.01/kizunaai/kizunaai.pmx";
	//wchar_t const* directory_path = L"E:�f��/�L�Y�i�A�C/KizunaAI_ver1.01/kizunaai/";
	//wchar_t const* file_path = L"E:�f��/�z�����C�u/�Ƃ��̂������mmd_ver2.1/�Ƃ��̂���.pmx";
	//wchar_t const* directory_path = L"E:�f��/�z�����C�u/�Ƃ��̂������mmd_ver2.1/";
	//wchar_t const* file_path = L"E:�f��/�z�����C�u/Laplus_220516_1/Laplus/PMX/Laplus_220516_1.pmx";
	//wchar_t const* directory_path = L"E:�f��/�z�����C�u/Laplus_220516_1/Laplus/sourceimages/";

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
	// vpd�̓ǂݍ���
	//

	//wchar_t const* pose_file_path = L"../../../3dmodel/�|�[�Y25/4.vpd";
	wchar_t const* pose_file_path = L"../../../3dmodel/Pose Pack 6 - Snorlaxin/9.vpd";
	std::ifstream pose_file{ pose_file_path };
	auto vpd_data = mmdl::load_vpd_data<std::vector<::vpd_data>>(pose_file);
	pose_file.close();


	//
	// vmd�̓ǂݍ���
	//

	//const wchar_t* motion_file_path = L"../../../3dmodel/�X�C�}�W/sweetmagic-right.vmd";
	const wchar_t* motion_file_path = L"../../../3dmodel/�T���}���_�[ ���[�V����(short)/�T���}���_�[ ���[�V����(short).vmd";
	std::ifstream motion_file{ motion_file_path,std::ios::binary };

	auto vmd_header = mmdl::load_vmd_header<::vmd_header>(motion_file);
	auto vmd_frame_data = mmdl::load_vmd_frame_data<std::vector<::vmd_frame_data>>(motion_file, vmd_header.frame_data_num);
	auto vmd_morph_data = mmdl::load_vmd_morph_data<std::vector<::vmd_morph_data>>(motion_file);
	motion_file.close();

	//
	// �N���A�o�����[
	// ���\�[�X�̐������ɕK�v
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
	// ���\�[�X
	//

	// FRAME_BUFFER_NUM�̐��̃t���[���o�b�t�@
	auto frame_buffer_resource = [&swap_chain]() {
		std::array<dx12w::resource_and_state, FRAME_BUFFER_NUM> result{};

		// �X���b�v�`�F�[������t���[���o�b�t�@���擾
		for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
		{
			ID3D12Resource* tmp = nullptr;
			swap_chain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&tmp));
			result[i] = std::make_pair(dx12w::release_unique_ptr<ID3D12Resource>{tmp}, D3D12_RESOURCE_STATE_COMMON);
		}

		return result;
	}();

	// �f�v�X�o�b�t�@
	auto depth_buffer = dx12w::create_commited_texture_resource(device.get(), DEPTH_BUFFER_FORMAT, WINDOW_WIDTH, WINDOW_HEIGHT,
		2, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depth_clear_value);

	// pmx�̒��_�o�b�t�@
	auto pmx_vertex_buffer_resource = [&device, &pmx_vertex]() {
		auto result = dx12w::create_commited_upload_buffer_resource(device.get(), sizeof(model_vertex_data) * pmx_vertex.size());

		model_vertex_data* tmp = nullptr;
		result.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		std::copy(pmx_vertex.begin(), pmx_vertex.end(), tmp);
		result.first->Unmap(0, nullptr);

		return result;
	}();

	// pmx�̃C���f�b�N�X�̃o�b�t�@
	auto pmx_index_buffer_resource = [&device, &pmx_surface]() {
		auto result = dx12w::create_commited_upload_buffer_resource(device.get(), sizeof(index) * pmx_surface.size());

		index* tmp = nullptr;
		result.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		std::copy(pmx_surface.begin(), pmx_surface.end(), tmp);
		result.first->Unmap(0, nullptr);

		return result;
	}();

	// model_data���}�b�v���郊�\�[�X
	auto model_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(model_data), 256));

	// camera_data���}�b�v���郊�\�[�X
	auto camera_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(camera_data), 256));

	// direction_light_data���}�b�v����p�̃��\�[�X
	auto direction_light_data_resource = dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(direction_light_data), 256));

	// pmx��`�悷�钸�_�V�F�[�_
	auto pmx_vertex_shader = []() {
		std::ifstream shader_file{ L"shader/VertexShader.cso",std::ios::binary };
		return dx12w::load_blob(shader_file);
	}();

	// pmx��`�悷��s�N�Z���V�F�[�_
	auto pmx_index_shader = []() {
		std::ifstream shader_file{ L"shader/PixelShader.cso",std::ios::binary };
		return dx12w::load_blob(shader_file);
	}();

	// 4x4�̔����e�N�X�`��
	auto white_texture_resource = get_fill_4x4_texture_resource(device, command_manager, PMX_TEXTURE_FORMAT, 255);

	// 4x4�̍����e�N�X�`��
	auto black_texture_resource = get_fill_4x4_texture_resource(device, command_manager, PMX_TEXTURE_FORMAT, 0);

	// �e�N�X�`���̃��\�[�X
	auto pmx_texture_resrouce = get_pmx_texture_resource(device, command_manager, pmx_texture_path, PMX_TEXTURE_FORMAT, directory_path);

	// �}�e���A���̃��\�[�X
	auto pmx_material_resource = get_pmx_material_resource(device, pmx_material);


	//
	// �f�B�X�N���v�^�q�[�v�ƃr���[
	//

	// �t���[���o�b�t�@�ɕ`�悷�鎞�̃����_�[�^�[�Q�b�g���w�肷��p�̃f�X�N���v�^�q�[�v
	auto frame_buffer_descriptor_heap_RTV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_NUM);
	// FRAME_BUFFER_NUM�̐��̃r���[���쐬
	for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
		dx12w::create_texture2D_RTV(device.get(), frame_buffer_descriptor_heap_RTV.get_CPU_handle(i), frame_buffer_resource[i].first.get(), FRAME_BUFFER_FORMAT, 0, 0);

	// �t���[���o�b�t�@�ɕ`�悷�鎞�̃f�v�X�o�b�t�@���w�肷��p�̃f�X�N���v�^�q�[�v
	auto depth_buffer_descriptor_heap_DSV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	dx12w::create_texture2D_DSV(device.get(), depth_buffer_descriptor_heap_DSV.get_CPU_handle(0), depth_buffer.first.get(), DEPTH_BUFFER_FORMAT, 0);

	// pmx�̃f�B�X�N���v�^�e�[�u��
	// ���[�g�V�O�l�`���������Ɏg�p���邪�A�f�X�N���v�^�̑傫�������肷�鎖�ɂ����p����
	std::vector<std::vector<dx12w::descriptor_range_type>> const pmx_descriptor_table{

		// 1�߂̃f�B�X�N���v�^�e�[�u��
		// �}�e���A���Ɉˑ����Ȃ����ʂ̃r���[
		{
			// ���f���̃f�[�^�A�J�����̃f�[�^�A���C�g�̃f�[�^��3�̒萔�o�b�t�@
			{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,3}
		},

		// 2�߂̃f�B�X�N���v�^�e�[�u��
		// �}�e���A���Ɉˑ��������ꂼ��̃r���[
		{
			// �}�e���A���̃e�N�X�`��
			{D3D12_DESCRIPTOR_RANGE_TYPE_SRV},
			// �f�B�t�[�Y�Ȃǂ̏��
			{D3D12_DESCRIPTOR_RANGE_TYPE_CBV},
			// ��Z�X�t�B�A�}�b�v�A���Z�X�t�B�A�}�b�v�A�g�D�[����3�̃V�F�[�_���\�[�X�r���[
			{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3}
		}
	};

	// pmx��`�悷�鎞�̋��ʂ̃r���[�̐��ƃ}�e���A�����Ƃɂ��ƂȂ�r���[�̐�
	UINT const pmx_common_view_num = dx12w::calc_descriptor_num(pmx_descriptor_table[0]);
	UINT const pmx_material_view_num = dx12w::calc_descriptor_num(pmx_descriptor_table[1]);

	// pmx��`�悷��p�̃f�X�N���v�^�q�[�v
	auto pmx_descriptor_heap_CBV_SRV_UAV = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		pmx_common_view_num + pmx_material_view_num * pmx_material.size());
	// 1.mode_data�̃r���[
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(0), model_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(model_data), 256));
	// 2.�J�����̃f�[�^�̃r���[
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(1), camera_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(camera_data), 256));
	// 3.���C�g�̃f�[�^�̃r���[
	dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(2), direction_light_data_resource.first.get(), dx12w::alignment<UINT64>(sizeof(direction_light_data), 256));
	// �}�e���A�����ƂɈقȂ�r���[���쐬���Ă�
	for (std::size_t i = 0; i < pmx_material.size(); i++)
	{
		// 4.�e�N�X�`���̃r���[
		dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 0),
			pmx_texture_resrouce[pmx_material_2[i].texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		// 5.�f�B�t���[�Y�J���[�Ȃǂ̏�񂪂���material_data�̃r���[
		dx12w::create_CBV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 1),
			pmx_material_resource[i].first.get(), dx12w::alignment<UINT64>(sizeof(material_data), 256));

		// 6.��Z�X�t�B�A�}�b�v�̃r���[
		// ��Z�X�t�B�A�}�b�v���g�p����ꍇ
		if (pmx_material_2[i].sphere_mode == sphere_mode::sph)
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 2),
				pmx_texture_resrouce[pmx_material_2[i].sphere_texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// ���Ȃ��ꍇ�͔��F�̃e�N�X�`��
		else
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 2),
				white_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// 7.���Z�X�t�B�A�}�b�v�̃r���[
		// ���Z�X�t�B�A�}�b�v���g�p����ꍇ
		if (pmx_material_2[i].sphere_mode == sphere_mode::spa)
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 3),
				pmx_texture_resrouce[pmx_material_2[i].sphere_texture_index].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// ���Ȃ��ꍇ�͍��F�̃e�N�X�`��
		else {
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 3),
				black_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// 8.�g�D�[���e�N�X�`���̃r���[
		// �񋤗L�̃g�D�[��
		// �C���f�b�N�X�ɖ����Ȓl�������Ă���ꍇ������̂Ŗ�������
		if (pmx_material_2[i].toon_type == toon_type::unshared && pmx_material_2[i].toon_texture < pmx_texture_path.size())
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 4),
				pmx_texture_resrouce[pmx_material_2[i].toon_texture].first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}
		// ����ȊO�͔��F�̃e�N�X�`��
		// ���L�̃g�D�[���ɂ��ẮuPmxEditor/_data/toon�v�ɉ摜�����������ǁA�Ƃ肠�����͖���
		else
		{
			dx12w::create_texture2D_SRV(device.get(), pmx_descriptor_heap_CBV_SRV_UAV.get_CPU_handle(3 + pmx_material_view_num * i + 4),
				white_texture_resource.first.get(), PMX_TEXTURE_FORMAT, 1, 0, 0, 0.f);
		}

		// pmx�ɂ̓T�u�e�N�X�`���̃X�t�B�A�}�b�v�̏������邪�A�Ƃ肠�����͖���
	}

	// pmxx�̒��_�o�b�t�@�̃r���[
	D3D12_VERTEX_BUFFER_VIEW pmx_vertex_buffer_view{
		.BufferLocation = pmx_vertex_buffer_resource.first->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(model_vertex_data) * pmx_vertex.size()),
		.StrideInBytes = static_cast<UINT>(sizeof(model_vertex_data)),
	};

	// pmx�̃C���f�b�N�X�o�b�t�@�̃r���[
	D3D12_INDEX_BUFFER_VIEW pmx_index_buffer_view{
		.BufferLocation = pmx_index_buffer_resource.first->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(index) * pmx_surface.size()),
		.Format = DXGI_FORMAT_R32_UINT
	};


	// imgui�p�̃f�B�X�N���v�^�q�[�v
	// �傫����1�ł������ۂ�?
	auto imgui_descriptor_heap = dx12w::create_descriptor_heap(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);


	//
	// ���[�g�V�O�l�`���ƃO���t�B�N�X�p�C�v���C��
	//

	// pmx�̃t���[���o�b�t�@�ɕ`�悷��ۂ̃��[�g�V�O�l�`��
	auto pmx_root_signature = dx12w::create_root_signature(device.get(), pmx_descriptor_table,
		{/*�ʏ�̃e�N�X�`���̃T���v��*/ {D3D12_FILTER_MIN_MAG_MIP_POINT ,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_TEXTURE_ADDRESS_MODE_WRAP,D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_COMPARISON_FUNC_NEVER},
		/*�g�D�[���p�T���v��*/{D3D12_FILTER_MIN_MAG_MIP_POINT ,D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,D3D12_TEXTURE_ADDRESS_MODE_CLAMP,D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,D3D12_COMPARISON_FUNC_NEVER} });

	// pmx���t���[���o�b�t�@�ɕ`�悷��ۂ̃O���t�B�N�X�p�C�v���C��
	auto pmx_graphics_pipeline_state = dx12w::create_graphics_pipeline(device.get(), pmx_root_signature.get(),
		{ { "POSITION",DXGI_FORMAT_R32G32B32_FLOAT },{ "NORMAL",DXGI_FORMAT_R32G32B32_FLOAT },{ "TEXCOORD",DXGI_FORMAT_R32G32_FLOAT },
		{ "BONEINDEX",DXGI_FORMAT_R32G32B32A32_UINT },{ "BONEWEIGHT",DXGI_FORMAT_R32G32B32A32_FLOAT } },
		{ FRAME_BUFFER_FORMAT }, { {pmx_vertex_shader.data(),pmx_vertex_shader.size()},{pmx_index_shader.data(),pmx_index_shader.size()} },
		true, true, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);


	//
	// Imgui�̐ݒ�
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

	// ������bullet_world����i��H�j�ɏ�����ƃG���[�͂��̂Œ���
	// TODO: bullet_world�Ƃ̉���̏��ɂ��Ă��!
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

	// �f�o�b�N�p�̃C���X�^���X������
	bullet_world.dynamics_world.setDebugDrawer(&debug_draw);
	

	//
	// ���̑��ݒ�
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

	// �q�{�[���̃C���f�b�N�X���擾���邽�߂̔z����擾
	auto const to_children_bone_index = get_to_children_bone_index(pmx_bone);

	// �{�[���̖��O����Ή�����{�[���̃C���f�b�N�X���擾����ۂɎg�p����
	auto const bone_name_to_bone_index = get_bone_name_to_bone_index(pmx_bone);

	// �{�[���̖��O����Ή�����{�[���̃A�j���[�V�������擾����ۂɎg�p����
	auto const bone_name_to_bone_motion_data = get_bone_name_to_bone_motion_data(vmd_frame_data);

	// ���[�t�̖��O�������Ή����郂�[�t�̃C���f�b�N�X����擾����ۂɎg�p����
	auto const morph_index_to_morph_vertex_index = get_morph_index_to_morph_vertex_index(pmx_vertex_morph);

	// ���[�t�̖��O����Ή����郂�[�t�̃A�j���[�V�������擾����ۂɎg�p����
	auto const morph_name_to_morph_motion_data = get_morph_name_to_morph_motion_data(vmd_morph_data);

	direction_light_data direction_light{
		.dir = direction_light_dir,
		.color = direction_light_color,
	};

	int frame_num = 0;
	bool auto_animation = false;
	bool use_vpd = false;

	// IK�̏����Ń{�[������]�����鐔
	int ik_rotation_num = -1;
	// IK�ɂ���čŌ�ɉ�]�������{�[���̊e�����𖳎�������]��\��
	bool check_ideal_rotation = false;
	// �c�]��]��K�p���邩�ǂ���
	bool is_residual = false;

	float offset_x = 0.f;
	float offset_y = 0.f;
	float offset_z = 0.f;

	auto prev_frame_time = std::chrono::system_clock::now();
	double elapsed = 0.0;

	// �������Z�p
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
		// �f�[�^�̍X�V
		//

		auto const view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		auto const proj = DirectX::XMMatrixPerspectiveFovLH(
			view_angle,
			asspect,
			camera_near_z,
			camera_far_z
		);

		// �{�[���̃f�[�^����������
		initialize_bone_data(bone_data);

		// �w�肳��Ă���{�[���ɓK���ȍs���ݒ�
		if (use_vpd)
			set_bone_data_from_vpd(bone_data, vpd_data, bone_name_to_bone_index);
		else
			set_bone_data_from_vmd(bone_data, bone_name_to_bone_motion_data, pmx_bone, bone_name_to_bone_index, frame_num);

		// IK�f�o�b�N�p
		{
			auto iter = bone_name_to_bone_index.find(L"�E���h�j");
			if (iter != bone_name_to_bone_index.end())
				bone_data[iter->second].transform += XMVECTOR{ offset_x, offset_y, offset_z };
		}

		// �������Z�f�o�b�O�p
		{
			bone_data[15].rotation = XMQuaternionMultiply(bone_data[15].rotation, XMQuaternionRotationRollPitchYaw(head_rotation_x, head_rotation_y, head_rotation_z));
		}

		auto const root_index = bone_name_to_bone_index.contains(L"�S�Ă̐e") ? bone_name_to_bone_index.at(L"�S�Ă̐e") : 0;

		// ���ꂼ��̐e�̃m�[�h�̉�]�A�ړ��̍s����q�֓`�d������
		set_to_world_matrix(bone_data, to_children_bone_index, root_index, XMMatrixIdentity(), pmx_bone);

		// IK
		int ik_rotation_counter = 0;
		recursive_aplly_ik(bone_data, root_index, to_children_bone_index, pmx_bone, ik_rotation_num, &ik_rotation_counter, check_ideal_rotation);

		// �t�^�̉���
		recursive_aplly_grant(bone_data, root_index, to_children_bone_index, pmx_bone);

		for (std::size_t i = 0; i < pmx_rigidbody.size(); i++)
		{
			// �{�[���Ǐ]
			if (pmx_rigidbody[i].rigidbody_type == 0)
			{
				auto bone_index = pmx_rigidbody[i].bone_index;

				// �Ώۃ{�[���̃��[���h���W�ւ̕ϊ��s��
				auto const to_world =
					XMMatrixTranslationFromVector(-XMLoadFloat3(&pmx_bone[bone_index].position)) *
					XMMatrixRotationQuaternion(bone_data[bone_index].rotation) *
					XMMatrixTranslationFromVector(XMLoadFloat3(&pmx_bone[bone_index].position)) *
					XMMatrixTranslationFromVector(bone_data[bone_index].transform) *
					bone_data[bone_index].to_world;

				// �Ώۃ{�[���̃��[���h���W�ւ̕ϊ��̉�]�݂̂�\���N�I�[�^�j�I��
				auto const to_world_rot = XMQuaternionRotationMatrix(to_world);

				// �{�[���̃��[���h���W
				auto const bone_world_pos = XMVector3Transform(XMLoadFloat3(&pmx_bone[bone_index].position), to_world);

				// ���[�J�����W�ł̃{�[���̍��W���獄�̂̍��W�ւ̃x�N�g��
				auto const bone_to_rigidbody_local_vec = XMVectorSubtract(XMLoadFloat3(&pmx_rigidbody[i].position), XMLoadFloat3(&pmx_bone[bone_index].position));

				// ���[���h���W�ł̃{�[���̍��W���獄�̂̍��W�ւ̃x�N�g��
				auto const bone_to_rigidbody_world_vec = XMVector3Rotate(bone_to_rigidbody_local_vec, to_world_rot);

				// ��]�̍��v
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

			// �S�Ă̍��̂��A�N�e�B�u�ɂ���
			bullet_rigidbody[i].rigidbody->activate(true);
		}

		//
		// �����G���W���̃V���~���[�V����
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
		// �����V���~���[�V�����̌��ʂ𔽉f������
		//

		// TODO:
		debug_draw.sphereData.clear();
		debug_draw.boxData.clear();
		debug_draw.capsuleData.clear();

		//
		// �����G���W���̌��ʂ�`�悷�邽�߂ɏ���
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
		// �}�b�v����
		//

		// ���_
		{
			model_vertex_data* tmp = nullptr;
			pmx_vertex_buffer_resource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
			std::copy(pmx_vertex.begin(), pmx_vertex.end(), tmp);

			
			// imgui���烂�[�t�����񂽂�����Ă���ꍇ
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
				// �A�j���[�V�������烂�[�t�����Ă�����ꍇ
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
			

			// �O���[�v���[�t
			for (auto const& morph : pmx_group_morph)
			{
				auto weight = get_morph_motion_weight(morph_name_to_morph_motion_data, morph.name, frame_num);

				// �O���[�v�Ɋ܂܂�钸�_���[�t�𑖍�
				for (auto const& gm : morph.data)
				{
					auto iter = morph_index_to_morph_vertex_index.find(gm.index);
					if (iter != morph_index_to_morph_vertex_index.end())
					{
						// ���_���[�t�Ɋ܂܂�钸�_�𑖍�
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
				// �������Z�ɂ���Ĉړ�����{�[��
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

					// ���[�J�����W�ł̍��̂���{�[���ւ̃x�N�g��
					auto const rigidbody_to_bone_local_vec = XMVectorSubtract(XMLoadFloat3(&pmx_bone[bone_index].position), XMLoadFloat3(&pmx_rigidbody[i].position));

					// �����V���~���[�V�����ɂ���Ĉʒu���C�����ꂽ���̂���݂����̂���{�[���ւ̃x�N�g����ϊ�
					auto const rigidbody_to_bone_world_vec = XMVector3Rotate(rigidbody_to_bone_local_vec, XMQuaternionMultiply(XMQuaternionMultiply(rot, XMQuaternionInverse(parent_rot)), XMQuaternionInverse(rigidbody_rot)));

					// �C����̃��[���h���W�ł̃{�[���̈ʒu
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
		// Imgui�̏���
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
		// pmx�̕`��̃R�}���h��ς�
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

		// ���ʂ̃r���[�̐ݒ�
		command_manager.get_list()->SetGraphicsRootDescriptorTable(0, pmx_descriptor_heap_CBV_SRV_UAV.get_GPU_handle(0));

		UINT index_offset = 0;
		for (std::size_t i = 0; i < pmx_material.size(); i++)
		{
			// �}�e���A�����Ƃ̃r���[�̐ݒ�
			// ���ʂ̃r���[�ɂ��Ă͂��łɐݒ肵�Ă���̂ŕK�v�Ȃ�
			command_manager.get_list()->SetGraphicsRootDescriptorTable(1, pmx_descriptor_heap_CBV_SRV_UAV.get_GPU_handle(pmx_common_view_num + pmx_material_view_num * i));

			// �}�e���A���̒��_�̃I�t�Z�b�g���w�肵�`��
			command_manager.get_list()->DrawIndexedInstanced(pmx_material_2[i].vertex_number, 1, index_offset, 0, 0);

			// �I�t�Z�b�g�̍X�V
			index_offset += pmx_material_2[i].vertex_number;
		}

		//
		// ���̂�Debug�p�̕`��
		//

		if (is_display_rigidbody)
		{
			command_manager.get_list()->RSSetViewports(1, &viewport);
			command_manager.get_list()->RSSetScissorRects(1, &scissor_rect);

			// �f�v�X�o�b�t�@�͂�������N���A���Ă���
			command_manager.get_list()->ClearDepthStencilView(depth_buffer_descriptor_heap_DSV.get_CPU_handle(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

			command_manager.get_list()->OMSetRenderTargets(1, &frame_buffer_cpu_handle, false, &depth_buffer_cpu_handle);

			debug_shape_pipeline->draw(command_manager.get_list(), *debug_box_resource);
			debug_shape_pipeline->draw(command_manager.get_list(), *debug_sphere_resoruce);
			debug_shape_pipeline->draw(command_manager.get_list(), *debug_capsule_resrouce);
		}


		//
		// Imgui�̕`��̃R�}���h��ς�
		//

		auto imgui_descriptor_heap_ptr = imgui_descriptor_heap.get();
		command_manager.get_list()->SetDescriptorHeaps(1, &imgui_descriptor_heap_ptr);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_manager.get_list());


		dx12w::resource_barrior(command_manager.get_list(), frame_buffer_resource[back_buffer_index], D3D12_RESOURCE_STATE_COMMON);
		dx12w::resource_barrior(command_manager.get_list(), depth_buffer, D3D12_RESOURCE_STATE_COMMON);


		//
		// �R�}���h���s
		//

		command_manager.get_list()->Close();
		command_manager.excute();
		command_manager.signal();

		command_manager.wait(0);

		swap_chain->Present(1, 0);


		//
		// ���̑�
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