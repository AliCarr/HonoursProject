#pragma once

#include "stdafx.h"

struct ParticleInformation
{
	XMFLOAT3 position;
	XMFLOAT3 colour;
	float  velocity;
	bool active;
};


class Particle
{
public:
	Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList);
	~Particle();

	float update(XMMATRIX&, float);


private:
	const float height = 1.f, width = 1.f;
	XMFLOAT3 position;
	float velocity;
	float energy;
	bool isActive;
	//std::unique_ptr<MeshGeometry> geo = nullptr;
	SubmeshGeometry gridSubmesh;

	GeometryGenerator geoGen;
	UINT gridVertexOffset;
	UINT gridIndexOffset;

	ParticleInformation mInfo;

	bool CreateParticle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList);

	GeometryGenerator::MeshData mesh;
};

