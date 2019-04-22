#pragma once
#pragma once

#include "stdafx.h"
#include <time.h>

class GPUParticleManager
{
public:
	GPUParticleManager(ComPtr<ID3D12Device>&, ID3D12GraphicsCommandList* ,ComPtr<ID3DBlob> , ComPtr<ID3D12PipelineState>&);
	~GPUParticleManager();

	void Render(ComPtr<ID3D12DescriptorHeap> &heap);
	void Execute();
	void Update(int, int);
	ID3D12Resource *pUavResource;

private:
	time_t mTime;

	static const int numberOfParticles = 2760;
	int currentNumberOfParticles = numberOfParticles;
	int currentAmountOfComputeWork;

	bool GenerateParticleMesh(ComPtr<ID3D12Device>&, ID3D12GraphicsCommandList*);
	void CreateBuffers(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>);
	void BuildDescriptors(UINT descriptorSize, ComPtr<ID3D12DescriptorHeap>&, ID3D12GraphicsCommandList*);
	void CreateRootSignatures();

	void BuildResources();
	void BuildPSO(ComPtr<ID3DBlob> mcsByteCode, ComPtr<ID3D12PipelineState > &mPSO);
	XMFLOAT3 StartingVelocity();
	XMFLOAT3 StartingPosition();

	unsigned long long totalVertexCount;
	
	MeshGeometry *mGeo = nullptr;
	std::vector<ParticleInfromation*> mParticles;
	GeometryGenerator generator;
	SubmeshGeometry boxSubmesh;
	Vertex vert[numberOfParticles];
	const float width = 0.008f;
	const float depth = 0.0004f;
	const UINT32 rows = 2;
	const UINT32 columns = 2;
	std::vector<ComputeData> particleInputeData;
	

	UINT m_srvUavDescriptorSize;
	bool whichHandle = false;
	
	UINT UAV = 1U;
	UINT SRV = 0U;
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;

	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
	ComPtr<ID3D12DescriptorHeap> mComputeHeap = nullptr;

	ComPtr<ID3D12Resource> m_particleBufferForDraw;
	ComPtr<ID3D12Resource> inputParticleBuffer;
	ComPtr<ID3D12Resource> outputParticleBuffer;
	ComPtr<ID3D12Resource> uploadParticleBuffer;

	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12RootSignature> mComputeRootSignature = nullptr;
	ComPtr<ID3D12Device> md3ddevice;
	ID3D12GraphicsCommandList* list;
};