#pragma once

#include "stdafx.h"
#include <stdio.h>
#include <time.h>

class ParticleManager
{
public:
	ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, 
					ID3D12GraphicsCommandList* commandList, 
					std::unique_ptr<MeshGeometry>&);

	~ParticleManager();

	void Update(float, 
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, 
				Microsoft::WRL::ComPtr<ID3D12Device> &);

	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, 
				ComPtr<ID3D12DescriptorHeap>&, 
				UINT, 
				Microsoft::WRL::ComPtr<ID3D12Device> &);

private: //Constants
	static const int numberOfParticles = 1000;

	//Constants for the particles mesh
	const float width = 0.004f;
	const float depth = 0.004f;
	const UINT32 rows = 2;
	const UINT32 columns = 2;

	//The smaller this is the faster it'll go
	const float speed = 100.0f;
	const float maxAcceleration = 10.0f;

	//Functions
	bool GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, 
							  ID3D12GraphicsCommandList *commandList);

	void UpdatePosition(int, 
						float, 
						UploadBuffer<Vertex>*);

	void ParticleReset(int);
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
};