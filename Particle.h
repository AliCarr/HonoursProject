#pragma once

#include "stdafx.h"

class Particle
{
public:
	Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList);
	~Particle();

	//Functions
	XMFLOAT3 update(XMMATRIX&, float);
	bool IsActive(){ return isActive; };


private:
	//Fixed Values
	const float height = 0.01f, width = 0.01f, velocity = 0.5f;

	//Particle Information
	XMFLOAT3 position;
	XMFLOAT3 colour;
	//float velocity;
	float energy;
	bool isActive;

	//D3D12 Variables
	SubmeshGeometry gridSubmesh;
	GeometryGenerator geoGen;
	UINT gridVertexOffset;
	UINT gridIndexOffset;
	GeometryGenerator::MeshData mesh;

	//Functions
	bool CreateParticle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList);

};

