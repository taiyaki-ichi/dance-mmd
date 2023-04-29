#pragma once
#include<fstream>
#include"../external/bullet3/src/btBulletCollisionCommon.h"
#include"../external/bullet3/src/btBulletDynamicsCommon.h"
#include<DirectXMath.h>
#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"obj_loader.hpp"
#include"struct.hpp"

using namespace DirectX;

constexpr std::size_t MAX_SHAPE_NUM = 256;

// 剛体を描画する際のシェーダに渡すデータ
struct shape_data
{
	DirectX::XMMATRIX transform{};
	std::array<float, 3> color{};
};

// 剛体のリソースなど
class shape_resource
{
	dx12w::resource_and_state vertexResource{};
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	UINT vertexNum{};

	dx12w::resource_and_state shapeDataConstantBuffer{};

	dx12w::descriptor_heap descriptorHeapCBVSRVUAV{};

	UINT shapeNum{};

public:
	shape_resource(ID3D12Device*, char const* fileName, ID3D12Resource* cameraDataResource);
	virtual ~shape_resource() = default;
	shape_resource(shape_resource&) = delete;
	shape_resource& operator=(shape_resource const&) = delete;
	shape_resource(shape_resource&&) = default;
	shape_resource& operator=(shape_resource&&) = default;

	template<typename Iter>
	void setShapeData(Iter first, Iter last);

	D3D12_VERTEX_BUFFER_VIEW const& getVertexBufferView() noexcept;
	UINT getVertexNum() const noexcept;
	dx12w::descriptor_heap& getDescriptorHeap() noexcept;
	UINT getShapeNum() const noexcept;
};

// 剛体を描画する際に使用するパイプラインなど
class shape_pipeline
{
	std::vector<std::uint8_t> vertexShader{};
	std::vector<std::uint8_t> pixelShader{};

	dx12w::release_unique_ptr<ID3D12RootSignature> rootSignature{};
	dx12w::release_unique_ptr<ID3D12PipelineState> graphicsPipeline{};

	UINT boxNum{};

public:
	shape_pipeline(ID3D12Device*, DXGI_FORMAT rendererFormat);
	virtual ~shape_pipeline() = default;
	shape_pipeline(shape_pipeline&) = delete;
	shape_pipeline& operator=(shape_pipeline const&) = delete;
	shape_pipeline(shape_pipeline&&) = default;
	shape_pipeline& operator=(shape_pipeline&&) = default;

	void draw(ID3D12GraphicsCommandList*, shape_resource& shapeResource);
};

// デバック用の描画をする際にbulletに渡す構造体
struct debug_draw : public btIDebugDraw
{
	std::vector<shape_data> sphereData{};
	std::vector<shape_data> boxData{};
	std::vector<shape_data> capsuleData{};

	void drawSphere(btScalar radius, const btTransform& transform, const btVector3& color) override;
	void drawSphere(const btVector3& p, btScalar radius, const btVector3& color) override;

	void drawBox(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color) override;
	void drawBox(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color) override;

	void drawCapsule(btScalar radius, btScalar halfHeight, int upAxis, const btTransform& transform, const btVector3& color) override;

	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override {}
	void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) override {}
	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {}
	void reportErrorWarning(const char* warningString) override {}
	void draw3dText(const btVector3& location, const char* textString) override {}

	int debug_mode;
	void setDebugMode(int debugMode) override;
	int getDebugMode() const override;
};





//
// 以下、実装
//


inline shape_resource::shape_resource(ID3D12Device* device, char const* fileName, ID3D12Resource* cameraDataResource)
{
	// 頂点データ
	{
		std::ifstream file{ fileName };
		auto vertexData = load_obj(file);

		vertexResource = dx12w::create_commited_upload_buffer_resource(device, sizeof(decltype(vertexData)::value_type) * vertexData.size());

		float* tmp = nullptr;
		vertexResource.first->Map(0, nullptr, reinterpret_cast<void**>(&tmp));
		std::copy(&vertexData[0][0], &vertexData[0][0] + vertexData.size() * 6, tmp);
		vertexResource.first->Unmap(0, nullptr);

		vertexBufferView = {
			.BufferLocation = vertexResource.first->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(sizeof(decltype(vertexData)::value_type) * vertexData.size()),
			.StrideInBytes = static_cast<UINT>(sizeof(decltype(vertexData)::value_type)),
		};

		vertexNum = vertexData.size();
	}

	// 定数バッファ
	{
		shapeDataConstantBuffer = dx12w::create_commited_upload_buffer_resource(device, dx12w::alignment<UINT64>(sizeof(shape_data) * MAX_SHAPE_NUM, 256));
	}

	// ディスクリプタヒープ
	{
		descriptorHeapCBVSRVUAV.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);

		dx12w::create_CBV(device, descriptorHeapCBVSRVUAV.get_CPU_handle(0), cameraDataResource, dx12w::alignment<UINT64>(sizeof(camera_data), 256));
		dx12w::create_CBV(device, descriptorHeapCBVSRVUAV.get_CPU_handle(1), shapeDataConstantBuffer.first.get(), dx12w::alignment<UINT64>(sizeof(shape_data) * MAX_SHAPE_NUM, 256));
	}
}

template<typename Iter>
inline void shape_resource::setShapeData(Iter first, Iter last)
{
	shape_data* shapeDataPtr = nullptr;
	shapeDataConstantBuffer.first->Map(0, nullptr, reinterpret_cast<void**>(&shapeDataPtr));

	std::copy(first, last, shapeDataPtr);
	shapeNum = std::distance(first, last);
}

inline D3D12_VERTEX_BUFFER_VIEW const& shape_resource::getVertexBufferView() noexcept
{
	return vertexBufferView;
}

inline UINT shape_resource::getVertexNum() const noexcept
{
	return vertexNum;
}

inline dx12w::descriptor_heap& shape_resource::getDescriptorHeap() noexcept
{
	return descriptorHeapCBVSRVUAV;
}

inline UINT shape_resource::getShapeNum() const noexcept
{
	return shapeNum;
}


inline shape_pipeline::shape_pipeline(ID3D12Device* device, DXGI_FORMAT rendererFormat)
{

	// 頂点シェーダ
	{
		std::ifstream shaderFile{ L"shader/ShapeVertexShader.cso",std::ios::binary };
		vertexShader = dx12w::load_blob(shaderFile);
	}

	// ピクセルシェーダ
	{
		std::ifstream shaderFile{ L"shader/ShapePixelShader.cso",std::ios::binary };
		pixelShader = dx12w::load_blob(shaderFile);
	}

	// ルートシグネチャ
	{
		rootSignature = dx12w::create_root_signature(device, { {{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,2}} }, {});
	}

	// グラフィックスパイプライン
	{
		graphicsPipeline = dx12w::create_graphics_pipeline(device, rootSignature.get(),
			{ { "POSITION",DXGI_FORMAT_R32G32B32_FLOAT },{ "NORMAL",DXGI_FORMAT_R32G32B32_FLOAT } },
			{ rendererFormat }, { {vertexShader.data(),vertexShader.size()},{pixelShader.data(),pixelShader.size()} },
			true, true, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	}
}

inline void shape_pipeline::draw(ID3D12GraphicsCommandList* list, shape_resource& shapeResource)
{
	list->SetGraphicsRootSignature(rootSignature.get());
	list->SetPipelineState(graphicsPipeline.get());
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		auto tmp = shapeResource.getDescriptorHeap().get();
		list->SetDescriptorHeaps(1, &tmp);
	}
	list->SetGraphicsRootDescriptorTable(0, shapeResource.getDescriptorHeap().get_GPU_handle(0));

	list->IASetVertexBuffers(0, 1, &shapeResource.getVertexBufferView());
	list->DrawInstanced(shapeResource.getVertexNum(), shapeResource.getShapeNum(), 0, 0);
}

inline void debug_draw::drawSphere(btScalar radius, const btTransform& transform, const btVector3& color)
{
	sphereData.emplace_back(
		XMMatrixScaling(radius, radius, radius) * XMMatrixTranslation(transform.getOrigin().x(), transform.getOrigin().y(), transform.getOrigin().z()),
		std::array<float, 3>{ static_cast<float>(color.x()), static_cast<float>(color.y()), static_cast<float>(color.z()) }
	);
}

inline void debug_draw::drawSphere(const btVector3& p, btScalar radius, const btVector3& color)
{
	sphereData.emplace_back(
		XMMatrixScaling(radius, radius, radius) * XMMatrixTranslation(p.x(), p.y(), p.z()),
		std::array<float, 3>{ static_cast<float>(color.x()), static_cast<float>(color.y()), static_cast<float>(color.z()) }
	);
}

inline void debug_draw::drawBox(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color)
{
	float x = bbMax[0] - bbMin[0];
	float y = bbMax[1] - bbMin[1];
	float z = bbMax[2] - bbMin[2];

	float centerX = (bbMax[0] + bbMin[0]) / 2.f;
	float centerY = (bbMax[1] + bbMin[1]) / 2.f;
	float centerZ = (bbMax[2] + bbMin[2]) / 2.f;

	boxData.emplace_back(
		XMMatrixScaling(x, y, z) * XMMatrixTranslation(centerX, centerY, centerZ),
		std::array<float, 3>{ static_cast<float>(color.x()), static_cast<float>(color.y()), static_cast<float>(color.z()) }
	);
}

inline void debug_draw::drawBox(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color)
{
	float x = bbMax[0] - bbMin[0];
	float y = bbMax[1] - bbMin[1];
	float z = bbMax[2] - bbMin[2];

	float centerX = (bbMax[0] + bbMin[0]) / 2.f;
	float centerY = (bbMax[1] + bbMin[1]) / 2.f;
	float centerZ = (bbMax[2] + bbMin[2]) / 2.f;

	XMVECTOR q{ trans.getRotation().x(),trans.getRotation().y(), trans.getRotation().z(), trans.getRotation().w() };

	boxData.emplace_back(
		XMMatrixScaling(x, y, z) * XMMatrixTranslation(centerX, centerY, centerZ) *
		XMMatrixRotationQuaternion(q) * XMMatrixTranslation(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z()),
		std::array<float, 3>{ static_cast<float>(color.x()), static_cast<float>(color.y()), static_cast<float>(color.z()) }
	);
}

inline void debug_draw::drawCapsule(btScalar radius, btScalar halfHeight, int upAxis, const btTransform& transform, const btVector3& color)
{
	auto scale = XMMatrixIdentity();
	// x軸が上
	if (upAxis == 0) {
		scale = XMMatrixScaling((radius + halfHeight) / 2.f, radius / 2.f, radius / 2.f);
	}
	// y軸が上
	else if (upAxis == 1)
	{
		scale = XMMatrixScaling(radius, (radius + halfHeight) / 2.f, radius);
	}
	// z軸が上
	else if (upAxis == 2)
	{
		scale = XMMatrixScaling(radius, radius, (radius + halfHeight) / 2.f);
	}

	XMVECTOR q{ transform.getRotation().x(),transform.getRotation().y(), transform.getRotation().z(), transform.getRotation().w() };

	capsuleData.emplace_back(
		scale * XMMatrixRotationQuaternion(q) * XMMatrixTranslation(transform.getOrigin().x(), transform.getOrigin().y(), transform.getOrigin().z()),
		std::array<float, 3>{ static_cast<float>(color.x()), static_cast<float>(color.y()), static_cast<float>(color.z()) }
	);
}

inline void debug_draw::setDebugMode(int debugMode)
{
	debug_mode = debugMode;
}

inline int debug_draw::getDebugMode() const
{
	return debug_mode;
}