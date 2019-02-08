#pragma once

#include "stdafx.h"
#include "Particle.h"


class ParticleManager
{
public:
	ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>&);
	~ParticleManager();

	void Update(XMMATRIX&, float);
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12DescriptorHeap>&, UINT );
	void UpdateCBuffers(std::unique_ptr<UploadBuffer<ObjectConstants>> &);
	std::unique_ptr<MeshGeometry> mGeo = nullptr;
	MeshGeometry GetMeshGeo() { return *mGeo; };
	int GetIndexCount();

	void UpdateGeometry(MeshGeometry&);

private:

	const int numberOfParticles = 1;
	
	//Particle* mParticle[];
	std::vector<Particle*> mParticles;
};

