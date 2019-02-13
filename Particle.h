#pragma once

#include "stdafx.h"
#include <random>

class Particle
{
public:
	Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList);
	~Particle();

	//Functions
	XMFLOAT3 update(float);
	bool IsActive(){ return isActive; };
	XMMATRIX World;
	MeshGeometry *Geo ;
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;

	UINT ObjCBIndex = -1;
	void updateGeo(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry> &meshGeo);
private:
	//Fixed Values
	const float height = 0.01f, width = 0.01f, velocity = 0.5f + (float)((rand() % 500) / 450);

	//Particle Information
	XMFLOAT3 position;
	XMFLOAT3 colour;
	//float velocity;
	float energy;
	bool isActive;
	
	std::random_device rd;
	
	//D3D12 Variables
	SubmeshGeometry gridSubmesh;
	GeometryGenerator geoGen;
	UINT gridVertexOffset;
	UINT gridIndexOffset;
	GeometryGenerator::MeshData mesh;

	//Functions
	bool CreateParticle( Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry> &meshGeo);

	
};

