#pragma once

#include "stdafx.h"

class Particle
{
public:
	Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, float offset);
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
	/*
	What do I want from teh particle class
	- Energy (how long it will live)
	- Position
	- Height and width
	- Is it Alive
	
	
	*/
};

