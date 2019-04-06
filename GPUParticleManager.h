#pragma once
#pragma once

#include "stdafx.h"
#include <time.h>

//struct ParticleInfromation
//{
//	//A unit of mesurement for the particles life
//	float energy;
//	float accelertaion;
//
//	XMFLOAT3 position;
//	//XMFLOAT3 colour;
//	XMFLOAT3 velocity;
//
//	bool isActive;
//	MeshGeometry* geo = nullptr;
//	std::unique_ptr<UploadBuffer<Vertex>> dynamicVB = nullptr;
//};

class GPUParticleManager
{
public:
	GPUParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, std::unique_ptr<MeshGeometry>& mesh);
	~GPUParticleManager();

	void initialise();
	void update();
	void render();
	void Execute(ID3D12GraphicsCommandList*, ComPtr<ID3D12PipelineState>, ComPtr<ID3D12RootSignature>, ComPtr<ID3D12DescriptorHeap>&);
	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
		UINT descriptorSize, ComPtr<ID3D12DescriptorHeap>&, ID3D12GraphicsCommandList*);

private:
	time_t mTime;
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;
	const int numOfParticles = 1000;
	
	
	
	void CreateBuffers(Microsoft::WRL::ComPtr<ID3D12Device> &, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void CreateRootSignatures(Microsoft::WRL::ComPtr<ID3D12Device> &device);
	
	bool GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList);
	void UpdatePosition(int, float, UploadBuffer<Vertex>*);
	void ParticleReset(int);
	void BuildResources();
	//void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
	//	UINT descriptorSize);

	unsigned long long totalVertexCount;
	static const int numberOfParticles = 1000;
	MeshGeometry *mGeo = nullptr;
	std::vector<ParticleInfromation*> mParticles;
	GeometryGenerator generator;
	SubmeshGeometry boxSubmesh;
	Vertex vert[numberOfParticles];
	const float width = 0.004f;
	const float depth = 0.004f;
	const UINT32 rows = 2;
	const UINT32 columns = 2;
	std::vector<ComputeData*> particleInputeData;
	
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;

	XMFLOAT3 StartingVelocity();
	XMFLOAT3 StartingPosition();
	UINT m_srvUavDescriptorSize;
	ComPtr<ID3D12Resource> m_particleBufferForDraw;

	ComPtr<ID3D12Resource> inputParticleBuffer;
	ComPtr<ID3D12Resource> outputParticleBuffer;
	ComPtr<ID3D12Resource> uploadParticleBuffer;

	ComPtr<ID3D12RootSignature> m_rootSignature;

	Microsoft::WRL::ComPtr<ID3D12Device> md3ddevice;
	ID3D12GraphicsCommandList* list;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuUav;

	const UINT SRV = 0U;
	const UINT UAV = 1U;
};