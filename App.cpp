#include "App.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		App theApp(hInstance);
		if (!theApp.Initialize())
			return 0;
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

App::App(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
	mControl = new Controls();
	mUI = new UI();
}

App::~App()
{
	if (mControl)
	{
		delete mControl;
		mControl = 0;
	}

	if (pManager)
	{
		delete pManager;
		pManager = 0;
	}

	if (mUI)
	{
		delete mUI;
		mUI = 0;
	}

	if (gpuPar)
	{
		delete gpuPar;
		gpuPar = 0;
	}

	if (mBoxGeo)
	{
		delete &mBoxGeo;
		mBoxGeo = 0;

	}	
}

bool App::Initialize()
{
	////DON'T CHANGE THIS UNTIL PSO FIXED!!!!
	switcher = true;

	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildPSO();

	pManager = new ParticleManager(md3dDevice, mCommandList.Get(), mBoxGeo2);
	gpuPar = new GPUParticleManager(md3dDevice, mCommandList.Get(), mBoxGeo, mComputeHeap, mcsByteCode, mPSO["compute"], mCommandQueue);
	mUI->GUIInit(MainWnd(), md3dDevice.Get(), mCbvHeap.Get());

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();
	return true;
}

void App::OnResize()
{
	D3DApp::OnResize();
	mControl->OnResize(AspectRatio());
}

void App::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	mControl->mCamera->Update();

	ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(mControl->mCamera->GetWorldViewProj()));
		objConstants.yPosiiton += gt.DeltaTime();
		objConstants.pulseColour = XMFLOAT4(1, 0, 0, 1);
	mObjectCB->CopyData(0, objConstants);

	mUI->GUIUpdate();
}

void App::Draw(const GameTimer& gt)
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO["renderPSO"].Get()));

		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		RecordRenderCommands();
		
		mUI->GUIRender(mCommandList);

	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	assert(mSwapChain);
	ThrowIfFailed(md3dDevice->GetDeviceRemovedReason());
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void App::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 13; //Originally 11, we're adding 2 more
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,IID_PPV_ARGS(&mCbvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC computeHeapDesc = {};
		computeHeapDesc.NumDescriptors = 3U;
		computeHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		computeHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&computeHeapDesc, IID_PPV_ARGS(&mComputeHeap)));

	mCbvHeap->SetName(L"Constant Buffer Heap");
	mComputeHeap->SetName(L"Compute Heap");
}

void App::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	// Offset to the ith object constant buffer in the buffer.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = 2000;
	srvDesc.Buffer.StructureByteStride = sizeof(ComputeData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	D3D12_RESOURCE_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D12_RESOURCE_DESC));
	bufferDesc.Alignment = 0;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	bufferDesc.Width = 2000 * sizeof(ComputeData);
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; //used for buffers as texture data can be located in them without creating a texture object
	bufferDesc.SampleDesc.Count = 1;

	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mInputBufferA));

	mInputBufferA->SetName(L"Draw Buffer");

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mCbvHeap->GetCPUDescriptorHandleForHeapStart(), 1U, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	md3dDevice->CreateShaderResourceView(mInputBufferA.Get(), &srvDesc, srvHandle);

}

void App::BuildRootSignature()
{
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE ranges[1];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
		cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

		slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
		slotRootParameter[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}




void App::RecordRenderCommands()
{
	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = CurrentBackBuffer();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	mCommandList->ResourceBarrier(1, &barrier);
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() , mComputeHeap.Get() };
	mCommandList->SetDescriptorHeaps(1, descriptorHeaps );

	if (switcher)
	{
		RecordComputeCommands();

		mCommandList->SetPipelineState(mPSO["renderPSO"].Get());
		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
		mCommandList->SetDescriptorHeaps(1, descriptorHeaps);

		RecordCopyCommands();

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(mCbvHeap->GetGPUDescriptorHandleForHeapStart(), 1U, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		mCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);

		gpuPar->Render(mCbvHeap);
	}

	if (!switcher)
	{
		mCommandList->SetPipelineState(mPSO["cpuPSO"].Get());
		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
		mCommandList->SetDescriptorHeaps(1, descriptorHeaps);
		pManager->Render(mCommandList, mCbvHeap, mCbvSrvUavDescriptorSize, md3dDevice);
	}

	mCommandList->ResourceBarrier(1, &barrier);
}
void App::RecordCopyCommands()
{
	gpuPar->CopyBuffers(mInputBufferA, mCommandList, mCommandQueue, mPSO["compute"]);
}
void App::RecordComputeCommands()
{
	gpuPar->Execute(mCommandList.Get(), mPSO["compute"], gpuPar->GetComputeRootSignature().Get(), mComputeHeap);
}
void App::CreateLists()
{

}





void App::OnMouseDown(WPARAM btnState, int x, int y)
{
	mControl->OnMouseDown(btnState, x, y, MainWnd());
}

void App::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void App::OnMouseMove(WPARAM btnState, int x, int y)
{
	mControl->OnMouseMove(btnState, x, y);
}

void App::OnKeyboardInput(const GameTimer& gt)
{
	mControl->OnKeyboardInput(gt.DeltaTime());
}


void App::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\colorVS.hlsl", nullptr, "VS", "vs_5_0");
	mvsCPUByteCode = d3dUtil::CompileShader(L"Shaders\\cpuParVS.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\colorPS.hlsl", nullptr, "PS", "ps_5_0");
	mcsByteCode = d3dUtil::CompileShader(L"Shaders\\particleCS.hlsl", nullptr, "UpdateWavesCS", "cs_5_0");

	mInputLayout =
	{
		//Semantic Name, Semantic Index, Format, Input slot, Aligned Byte Offset, Input Slot Class, Instance Data Step Rate
		{ "POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,   0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEX",      0,       DXGI_FORMAT_R32G32_FLOAT, 0,  12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "ID",		  0,		   DXGI_FORMAT_R16_UINT, 0,  36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void App::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO["renderPSO"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC cpuPsoDesc;
	ZeroMemory(&cpuPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	cpuPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	cpuPsoDesc.pRootSignature = mRootSignature.Get();

	cpuPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsCPUByteCode->GetBufferPointer()),
		mvsCPUByteCode->GetBufferSize()
	};
	cpuPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};

	cpuPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	cpuPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	cpuPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	cpuPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	cpuPsoDesc.SampleMask = UINT_MAX;
	cpuPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	cpuPsoDesc.NumRenderTargets = 1;
	cpuPsoDesc.RTVFormats[0] = mBackBufferFormat;
	cpuPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	cpuPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	cpuPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO["cpuPSO"])));

}

