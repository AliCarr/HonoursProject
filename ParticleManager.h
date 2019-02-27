#pragma once

#include "stdafx.h"
#include "Particle.h"

struct ParticleInfromation
{
	const float height = 0.01f, width = 0.01f;
	XMFLOAT3 position;
	XMFLOAT3 colour;
	float energy;
	bool isActive;
	MeshGeometry* geo = nullptr;
	UINT IndexCount;
	UINT StartIndexLocation;
	int BaseVertexLocation;
	UINT vbByteSize;
	std::unique_ptr<UploadBuffer<Vertex>> dynamicVB = nullptr;
};

class ParticleManager
{
public:
	ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>&);
	~ParticleManager();

	void Update(XMMATRIX&, float, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &, Microsoft::WRL::ComPtr<ID3D12Device> &);
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12DescriptorHeap>&, UINT, Microsoft::WRL::ComPtr<ID3D12Device> &device);
	void UpdateCBuffers(std::unique_ptr<UploadBuffer<ObjectConstants>> &);
	std::unique_ptr<MeshGeometry> mGeo = nullptr;
	MeshGeometry GetMeshGeo() { return *mGeo; };
	int GetIndexCount();

	void UpdateGeometry(MeshGeometry&);

private:
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;
	unsigned long long totalVertexCount;

	static const int numberOfParticles = 4;

	//ParticleInfromation* mParticle[numberOfParticles];
	//Particle* mParticle[];
	std::vector<ParticleInfromation*> mParticles;
};