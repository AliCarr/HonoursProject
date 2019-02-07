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
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	MeshGeometry* Geo = nullptr;
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
private:
	//Fixed Values
	const float height = 0.01f, width = 0.01f, velocity = 0.5f + ((rand()%50)/40);

	//Particle Information
	XMFLOAT3 position;
	XMFLOAT3 colour;
	//float velocity;
	float energy;
	bool isActive;
	
	UINT ObjCBIndex = -1;

	
	//D3D12 Variables
	SubmeshGeometry gridSubmesh;
	GeometryGenerator geoGen;
	UINT gridVertexOffset;
	UINT gridIndexOffset;
	GeometryGenerator::MeshData mesh;

	//Functions
	bool CreateParticle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList);

};

