#pragma once

#include "stdafx.h"
#include "Particle.h"

struct ParticleItems
{
	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;

	const float height = 0.01f, width = 0.01f, velocity = 0.5f + (float)((rand() % 500) / 450);

	//Particle Information
	XMFLOAT3 position;
	XMFLOAT3 colour;
};

class ParticleManager
{
public:
	ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>&);
	~ParticleManager();

	void Update(XMMATRIX&, float);
<<<<<<< HEAD
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12DescriptorHeap>&, UINT, Microsoft::WRL::ComPtr<ID3D12Device> &device, std::unique_ptr<MeshGeometry>& mesh);
=======
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12DescriptorHeap>&, UINT );
>>>>>>> parent of 5b3725a... Switch in render item
	void UpdateCBuffers(std::unique_ptr<UploadBuffer<ObjectConstants>> &);
	std::unique_ptr<MeshGeometry> mGeo = nullptr;
	MeshGeometry GetMeshGeo() { return *mGeo; };
	int GetIndexCount();

	void UpdateGeometry(MeshGeometry&);

private:

<<<<<<< HEAD
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData meshs;
	const int numberOfParticles = 30;

	//D3D12 Variables
	SubmeshGeometry gridSubmesh;

	std::unique_ptr<MeshGeometry> meshGeo = nullptr;
	UINT gridVertexOffset;
	UINT gridIndexOffset;
	GeometryGenerator::MeshData meshing;

	const int numberOfParticles = 1;

=======
	const int numberOfParticles = 1;
>>>>>>> parent of 8d743fa... Multiple Particles
	
	//Particle* mParticle[];
	std::vector<std::unique_ptr<ParticleItems>> mParticles;
};

