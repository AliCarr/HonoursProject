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
	void Render(ID3D12GraphicsCommandList *commandList, ComPtr<ID3D12DescriptorHeap>&, UINT, Microsoft::WRL::ComPtr<ID3D12Device> &device);

	MeshGeometry *mGeo = nullptr;
	MeshGeometry GetMeshGeo() { return *mGeo; };

	int theOffset;
private:
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;

	unsigned long long totalVertexCount;
	static const int numberOfParticles = 1024;

	std::vector<ParticleInfromation*> mParticles;
	GeometryGenerator generator;
	SubmeshGeometry boxSubmesh;
	Vertex vert[numberOfParticles];

	//Constants for the particles mesh
	const float width = 0.1f;
	const float depth = 0.1f;
	const float rows = 2.0f;
	const float columns = 2.0f;

	//The smaller this is the faster it'll go
	const float speed = 10;
	const float maxAcceleration = 10;

	time_t mTime;

	XMFLOAT3 StartingVelocity();
	XMFLOAT3 StartingPosition();
	XMFLOAT3 gravity;

	bool GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList);
	void UpdatePosition(int, float, UploadBuffer<Vertex>*);
	void ParticleReset(int);
};