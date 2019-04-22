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

	parNum = 100;
	amountOfComWork = 1;
	activeSystem = CPU;
	CPUActive = true;
	GPUActive = false;
	ACActive = false;
}

void UI::GUIUpdate()
{
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	bool t = true;
	colours[0] = false;
	colours[1] = false;
	static bool no_titlebar = false;
	static bool no_scrollbar = false;
	static bool no_menu = false;
	static bool no_move = false;
	static bool no_resize = false;
	static bool no_collapse = false;
	static bool no_close = false;
	static bool no_nav = false;
	static bool no_background = false;
	static bool no_bring_to_front = false;

	ImGuiWindowFlags window_flags = 0;
	if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
	if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
	if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
	if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
	if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
	if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
	if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

	if (!ImGui::Begin("ImGui Demo", &t, window_flags))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::PushItemWidth(ImGui::GetFontSize() * -12);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Colours"))
		{
			ImGui::MenuItem("Red", NULL, &colours[0]);
			ImGui::MenuItem("Blue", NULL, &colours[1]);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Red", NULL, &colours[0]);
			ImGui::MenuItem("Blue", NULL, &colours[1]);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();

		if (ImGui::CollapsingHeader("TestControls"))
		{
			ImGui::SliderInt("Amount of compute work", &amountOfComWork, 1, maxWork);
		}
		if (ImGui::CollapsingHeader("Systems"))
		{
			if (ImGui::Checkbox("CPU System", &CPUActive))
			{
				GPUActive = false;
				ACActive = false;
				activeSystem = CPU;
			}

			if (ImGui::Checkbox("GPU System", &GPUActive))
			{
				CPUActive = false;
				ACActive = false;
				activeSystem = GPU;
			}

			if (ImGui::Checkbox("AC System", &ACActive))
			{
				GPUActive = false;
				CPUActive = false;
				activeSystem = AC;
			}
		}
		if (ImGui::CollapsingHeader("Attributes"))
		{
			ImGui::SliderInt("Number Of Particles", &parNum, 1, maxParicles);
		}

		if (ImGui::CollapsingHeader("Information"))
		{
			ImGui::Text("Hello and welcome to Alastair Carr's fourth");
			ImGui::Text("year honours project; An Investigation into");
			ImGui::Text("the Benefits of Asynchronous Compute. This");
			ImGui::Text("application offers three methods of handling ");
			ImGui::Text("the computational workload of a particle ");
			ImGui::Text("system, a CPU side system, a GPU side ");
			ImGui::Text("system, and finally a system that runs using ");
			ImGui::Text("Asynchronous Compute. The UI provides all ");
			ImGui::Text("controls needed for testing, changing the ");
			ImGui::Text("visuals, and some other small tweaks. Enjoy! ");
			ImGui::Text("A.C.");
		}

		if (!ACActive && !GPUActive && !CPUActive)
			CPUActive = true;
	}

	ImGui::End();
}

void UI::GUIRender(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list)
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { uiHeap.Get() };
	list->SetDescriptorHeaps(1, descriptorHeaps);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list.Get());
}