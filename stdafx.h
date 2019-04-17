#pragma once

#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/GeometryGenerator.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

#define MAX_PARTICLES 1000
#define WIDTH 0.004f
#define DEPTH 0.004f
#define ROWS 2
#define COLUMNS 2
#define MAX_SPEED 100.f
#define MAX_ACCELERATION 10.f

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT4 Color;
	UINT id;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT4 pulseColour;
	float yPosiiton;
};

struct ComputeData
{
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	XMFLOAT3 acceleration;
	float energy;
};

struct ParticleInfromation
{
	//A unit of mesurement for the particles life
	float energy;
	float accelertaion;

	XMFLOAT3 position;
	//XMFLOAT3 colour;
	XMFLOAT3 velocity;

	bool isActive;
	MeshGeometry* geo = nullptr;
	std::unique_ptr<UploadBuffer<Vertex>> dynamicVB = nullptr;
};