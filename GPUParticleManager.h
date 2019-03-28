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
	ComPtr<ID3D12Resource> m_particleBuffer0Upload;
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;

	XMFLOAT3 StartingVelocity();
	XMFLOAT3 StartingPosition();
	UINT m_srvUavDescriptorSize;
	ComPtr<ID3D12Resource> m_particleBufferForDraw;

	enum DescriptorHeapIndex : UINT32
	{
		UavParticlePosVelo0,
		UavParticlePosVelo1,
		SrvParticlePosVelo0,
		SrvParticlePosVelo1,
		SrvParticleForDraw,
		DescriptorCount
	};

	ComPtr<ID3D12Resource> m_particleBuffer0;

	ComPtr<ID3D12RootSignature> m_rootSignature;
};

