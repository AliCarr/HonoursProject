#pragma once

#include "stdafx.h"

//Needed for Srand
#include <stdio.h>
#include <time.h>
#include "UI.h"

class ParticleManager
{
public:
	ParticleManager(ComPtr<ID3D12Device>&, ID3D12GraphicsCommandList*, std::unique_ptr<MeshGeometry>&);
	~ParticleManager();

	void Update(float, int, DirectX::XMMATRIX, UI &mUi);
	void Render(ComPtr<ID3D12DescriptorHeap>&);

private: 
	//System Constants
	static const int numberOfParticles = 6000;

	//Particle Constants
	const float width = 0.008f;
	const float depth = 0.008f;
	const UINT32 rows = 2;
	const UINT32 columns = 2;
	const float speed = 60.0f;
	const float maxAcceleration = 10.0f;

	//System Information
	int currentNumberOfParticles = 2046;

	//Functions
	bool GenerateParticleMesh(ComPtr<ID3D12Device>&, ID3D12GraphicsCommandList*);
	void UpdatePosition(int, float, UploadBuffer<Vertex>*, UI &mUi);
	void ParticleReset(int);
	void BuildHeap();
	void BuildPSO();
	void BuildRootSignature();
	XMFLOAT3 StartingVelocity();
	XMFLOAT3 StartingPosition();

	//Collections
	std::vector<ParticleInfromation*> mParticles;
	Vertex vert[numberOfParticles];

	//Dx12 Objects
	MeshGeometry *mGeo = nullptr;
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;
	unsigned long long totalVertexCount;
	GeometryGenerator generator;
	SubmeshGeometry boxSubmesh;
	ID3D12GraphicsCommandList* list;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	ComPtr<ID3D12Device> md3ddevice;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	ComPtr<ID3D12RootSignature> rootSig = nullptr;
	ComPtr<ID3D12PipelineState> pso;

	//srand time
	time_t mTime;
};