#pragma once

#include "stdafx.h"

class Particle
{
public:
	Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList);
	~Particle();




private:
	const float height = 1.0f, width = 1.0f;
	XMFLOAT3 position;
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

