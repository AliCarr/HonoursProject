#pragma once

#include "stdafx.h"
#include <stdio.h>
#include <time.h>

class ParticleManager
{
public:
	ParticleManager(ComPtr<ID3D12Device>&, ID3D12GraphicsCommandList*, std::unique_ptr<MeshGeometry>&);
	~ParticleManager();

	void Update(float, int, DirectX::XMMATRIX);
	void Render(ComPtr<ID3D12DescriptorHeap>&);

private: 
	static const int numberOfParticles = 2046;
	int currentNumberOfParticles = 2046;

	const float width = 0.004f;
	const float depth = 0.004f;
	const UINT32 rows = 2;
	const UINT32 columns = 2;

	//The smaller this is the faster it'll go
	const float speed = 100.0f;
	const float maxAcceleration = 10.0f;

	//Functions
	bool GenerateParticleMesh(ComPtr<ID3D12Device>&, ID3D12GraphicsCommandList*);
	void UpdatePosition(int, float, UploadBuffer<Vertex>*);
	void ParticleReset(int);
	void BuildHeap();
	void BuildPSO();
	void BuildRootSignature();
	XMFLOAT3 StartingVelocity();
	XMFLOAT3 StartingPosition();

	//Pointers
	std::vector<ParticleInfromation*> mParticles;
	MeshGeometry *mGeo = nullptr;

	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;
	unsigned long long totalVertexCount;
	GeometryGenerator generator;
	SubmeshGeometry boxSubmesh;
	Vertex vert[numberOfParticles];
	time_t mTime;
	ID3D12GraphicsCommandList* list;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	ComPtr<ID3D12Device> md3ddevice;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	ComPtr<ID3D12RootSignature> rootSig = nullptr;
	ComPtr<ID3D12PipelineState> pso;
};