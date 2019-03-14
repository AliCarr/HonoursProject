#pragma once


#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "imgui-master/imgui.h"
#include "imgui-master/imgui_impl_dx12.h"
#include "imgui-master/imgui_impl_win32.h"


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT4 Color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT4 pulseColour;
	float yPosiiton;
};
