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
	void GUIInit(HWND, ID3D12Device*, ID3D12DescriptorHeap*);
	void GUIUpdate();
	void GUIRender(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>);
	int numberOfParticles;

private:
	ComPtr<ID3D12DescriptorHeap> uiHeap = nullptr;
	
};

