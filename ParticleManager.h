#pragma once

#include "stdafx.h"
#include <stdio.h>
#include <time.h>

struct ParticleInfromation
{
	//A unit of mesurement for the particles life
	float energy;
	float accelertaion;

	XMFLOAT3 position;
	//XMFLOAT3 colour;
	XMFLOAT3 velocity;
	
	bool isActive;
	MeshGeometry* geo = nullptr;
	std::unique_ptr<UploadBuffer<Vertex>> dynamicVB = nullptr;
};

class ParticleManager
{
public:
	ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, std::unique_ptr<MeshGeometry>&);
	~ParticleManager();

	void Update(XMMATRIX&, float, ID3D12GraphicsCommandList* commandList, Microsoft::WRL::ComPtr<ID3D12Device> &);
	void Render(ID3D12GraphicsCommandList *commandList, ComPtr<ID3D12DescriptorHeap>&, UINT, Microsoft::WRL::ComPtr<ID3D12Device> &device, ComPtr<ID3D12PipelineState>);

	MeshGeometry *mGeo = nullptr;
	MeshGeometry GetMeshGeo() { return *mGeo; };

	int theOffset;

	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer;

private:
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;

	unsigned long long totalVertexCount;
	static const int numberOfParticles = 32;

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

	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUploadBuffer;

	//Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer;
};