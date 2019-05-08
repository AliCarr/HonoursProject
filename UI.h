#pragma once
#include "imgui-master/imgui.h"
#include "imgui-master/imgui_impl_dx12.h"
#include "imgui-master/imgui_impl_win32.h"
#include "stdafx.h"

class UI
{
public:
	UI();
	~UI();

	//Functions
	void GUIInit(HWND, ID3D12Device*, ID3D12DescriptorHeap*);
	void GUIUpdate();
	void GUIRender(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>);
	XMVECTORF32 GetColour();

private:
	//Dx12 Objects
	ComPtr<ID3D12DescriptorHeap> uiHeap = nullptr;

	//System Control Variables 
	bool colours[16];
	int colour;
	bool CPUActive, GPUActive, ACActive;
	enum Systems { CPU, GPU, AC };
	Systems activeSystem;
	int parNum, amountOfComWork;
	const int maxParicles = 6000;
	const int maxWork = 4000;
	
public:
	//Getters
	Systems GetSystem() { return activeSystem; };
	int GetNumberOfParticles() { return parNum; };
	int GetComputeWorkAmount() { return amountOfComWork; };
};

