#pragma once
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader-master\tiny_obj_loader.h"
#include "stdafx.h"
#include "Scene.h"
#include "ParticleManager.h"
#include "Controls.h"
#include "Common/Camera.h"
#include "UI.h"
#include "GPUParticleManager.h"
#include "ACParticleSystem.h"

class App : public D3DApp
{
public:
	App(HINSTANCE hInstance);
	App(const App& rhs) = delete;
	App& operator=(const App& rhs) = delete;
	~App();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	void OnKeyboardInput(const GameTimer& gt);

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSO();

private:

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mImGuiRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	
	ComPtr<ID3D12DescriptorHeap> mImGUIHeap = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;
	ComPtr<ID3DBlob> mcsByteCode = nullptr;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSO;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
	std::unique_ptr<MeshGeometry> mBoxGeo2 = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	POINT mLastMousePos;

	const std::string MODEL_PATH = "Assets/Mount Wario.obj";
	const std::string TEXTURE_PATH = "Assets/chalet.jpg";

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Controls *mControl;
	ParticleManager* pManager;
	UI *mUI;
	GPUParticleManager* gpuPar;

	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBufferA = nullptr;
	bool switcher;

	enum Systems { CPU, GPU, AC };
	Systems currentSystem;

	void RecordRenderCommands();

	float timer;

	ACParticleSystem* acSystem;
};