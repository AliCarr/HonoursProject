#include "UI.h"



UI::UI()
{
}


UI::~UI()
{
}

void UI::GUIInit(HWND wnd, ID3D12Device* device, ID3D12DescriptorHeap *heap)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init(wnd);

	ImGui_ImplDX12_Init(device, 
						3, 
						DXGI_FORMAT_R8G8B8A8_UNORM, 
						heap->GetCPUDescriptorHandleForHeapStart(), 
						heap->GetGPUDescriptorHandleForHeapStart());

	ImGui::StyleColorsDark();

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

	bool show = true;
	ImGui::ShowDemoWindow(&show);

	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
	ImGui::Checkbox("Demo Window", &show);      // Edit bools storing our window open/close state
	ImGui::Checkbox("Another Window", &show);

	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
	ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

	if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		counter++;

	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void UI::GUIRender()
{
	
}