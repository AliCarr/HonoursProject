#include "UI.h"



UI::UI()
{
}


UI::~UI()
{
}

void UI::GUIInit(HWND wnd, ID3D12Device* device, ID3D12DescriptorHeap *heap)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&uiHeap));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(wnd);

	ImGui_ImplDX12_Init(device, 
						1, 
						DXGI_FORMAT_R8G8B8A8_UNORM, 
						uiHeap->GetCPUDescriptorHandleForHeapStart(),
						uiHeap->GetGPUDescriptorHandleForHeapStart());


	//Pulled from example, will likely not be used
	bool show_demo_window = true;
	bool show_another_window = false;

}

void UI::GUIUpdate()
{
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	//bool show = true;
	//ImGui::ShowDemoWindow(&show);

	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		float current = 2;
		ImGui::Text("Number of Particles");
		ImGui::SliderFloat("Number", &current, 1, 1200);
		
	ImGui::End();
}

void UI::GUIRender(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list)
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { uiHeap.Get() };
	list->SetDescriptorHeaps(1, descriptorHeaps);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list.Get());
}