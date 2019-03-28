#pragma once

#include "stdafx.h"
#include <stdio.h>
#include <time.h>


class ParticleManager
{
public:
	ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, std::unique_ptr<MeshGeometry>&);
	~ParticleManager();

	void Update( float, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> &);
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12DescriptorHeap>&, UINT, Microsoft::WRL::ComPtr<ID3D12Device> &);

	MeshGeometry *mGeo = nullptr;
	MeshGeometry GetMeshGeo() { return *mGeo; };

	int theOffset;

	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	ParticleInfromation* getPar(int c) { return mParticles.at(c); };
    int GetNumberOfParticles(){ return numberOfParticles; };

private:
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;

	unsigned long long totalVertexCount;
	static const int numberOfParticles = 1000;

	std::vector<ParticleInfromation*> mParticles;
	GeometryGenerator generator;
	SubmeshGeometry boxSubmesh;
	Vertex vert[numberOfParticles];

	//Constants for the particles mesh
	const float width = 0.004f;
	const float depth = 0.004f;
	const UINT32 rows = 2;
	const UINT32 columns = 2;

	//The smaller this is the faster it'll go
	const float speed = 100.0f;
	const float maxAcceleration = 10.0f;

	time_t mTime;

	XMFLOAT3 StartingVelocity();
	XMFLOAT3 StartingPosition();
	XMFLOAT3 gravity;

	bool GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList);
	void UpdatePosition(int, float, UploadBuffer<Vertex>*);
	void ParticleReset(int);

	std::vector<ComputeData*> particleInputeData;

	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUploadBuffer = nullptr;

	//Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer;
};