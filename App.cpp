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
}

bool App::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));


	


	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildPSO();
	
	//Setting pointer for the Particle Manager has to be done here, so that set up has been done
	pManager = new ParticleManager(md3dDevice, 
								   mCommandList.Get(), 
								   mBoxGeo);

	gpuPar = new GPUParticleManager(md3dDevice,
		mCommandList.Get(),
		mBoxGeo);

	ThrowIfFailed(md3dDevice->GetDeviceRemovedReason());

	//auto mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto srvCpuStart = mComputeHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mComputeHeap->GetGPUDescriptorHandleForHeapStart();

	gpuPar->BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, 0, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
					         CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, 0, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
							 md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), mComputeHeap, mCommandList.Get());

	//BuildBuffers();

	ThrowIfFailed(md3dDevice->GetDeviceRemovedReason());

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
	ThrowIfFailed(md3dDevice->GetDeviceRemovedReason());
	pManager->Update(gt.DeltaTime(), mCommandList, md3dDevice);

	ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(mControl->mCamera->GetWorldViewProj()));
		objConstants.yPosiiton += gt.DeltaTime();
		objConstants.pulseColour = XMFLOAT4(1, 0, 0, 1);
	mObjectCB->CopyData(0, objConstants);
}

void App::Draw(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
	ThrowIfFailed(md3dDevice->GetDeviceRemovedReason());
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO["renderPSO"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

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

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() , mComputeHeap.Get()};
	mCommandList->SetDescriptorHeaps(1, descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());


	pManager->Render(mCommandList, mCbvHeap, mCbvSrvUavDescriptorSize, md3dDevice);
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &barrier);
	mCommandList->SetDescriptorHeaps(1, descriptorHeaps + 1);
	mCommandList->SetDescriptorHeaps(1, descriptorHeaps);
	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	assert(mSwapChain);
	ThrowIfFailed(md3dDevice->GetDeviceRemovedReason());
	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();

}

void App::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 11; //Originally 11, we're adding 2 more
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,IID_PPV_ARGS(&mCbvHeap)));
	mCbvHeap->SetName(L"Constant Buffer Heap");

	UINT descNumTotal = 2;

	D3D12_DESCRIPTOR_HEAP_DESC computeHeapDesc = {};
	computeHeapDesc.NumDescriptors = descNumTotal;
	computeHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	computeHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//computeHeapDesc.NodeMask = 0;
	
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&computeHeapDesc, IID_PPV_ARGS(&mComputeHeap)));
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
}

void App::BuildRootSignature()
{
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
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

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER computeSlotRootParameter[2];

	CD3DX12_DESCRIPTOR_RANGE ranges[2];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);


	// Perfomance TIP: Order from most frequent to least frequent.
	//computeSlotRootParameter[0].InitAsShaderResourceView(0);
	//computeSlotRootParameter[1].InitAsUnorderedAccessView(0);
	computeSlotRootParameter[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);


	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC computeRootSigDesc(2, computeSlotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> computeSerializedRootSig = nullptr;
	ComPtr<ID3DBlob> computeErrorBlob = nullptr;
	HRESULT computehr = D3D12SerializeRootSignature(&computeRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		computeSerializedRootSig.GetAddressOf(), computeErrorBlob.GetAddressOf());

	if (computeErrorBlob != nullptr)
	{
		::OutputDebugStringA((char*)computeErrorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		computeSerializedRootSig->GetBufferPointer(),
		computeSerializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mComputeRootSignature.GetAddressOf())));
}

void App::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\colorVS.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\colorPS.hlsl", nullptr, "PS", "ps_5_0");
	mcsByteCode = d3dUtil::CompileShader(L"Shaders\\particleCS.hlsl", nullptr, "UpdateWavesCS", "cs_5_0");

	mInputLayout =
	{
		//Semantic Name, Semantic Index, Format, Input slot, Aligned Byte Offset, Input Slot Class, Instance Data Step Rate
		{ "POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,   0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEX",      0,       DXGI_FORMAT_R32G32_FLOAT, 0,  12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePSO = {};
	//ZeroMemory(&computePSO, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	computePSO.pRootSignature = mComputeRootSignature.Get();
	computePSO.CS =
	{
		reinterpret_cast<BYTE*>(mcsByteCode->GetBufferPointer()), 
		mcsByteCode->GetBufferSize()
	};
	computePSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateComputePipelineState(&computePSO, IID_PPV_ARGS(&mPSO["compute"])));
	
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

void App::BuildBuffers()
{
	// Generate some data.
	std::vector<ComputeData> dataA(pManager->GetNumberOfParticles());
	std::vector<ComputeData> dataB(pManager->GetNumberOfParticles());
	for (int i = 0; i < pManager->GetNumberOfParticles(); ++i)
	{
		dataA.at(i).initialPosition = { 0, 0, 0 };
		dataA.at(i).position = { 0, 0, 0 };
		dataA.at(i).velocity = { 0, 0, 0 };
	}

	UINT64 byteSize = dataA.size() * sizeof(ComputeData);

	// Create some buffers to be used as SRVs.
	mInputBufferA = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		dataA.data(),
		byteSize,
		mInputUploadBufferA);

	mInputBufferB = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		dataB.data(),
		byteSize,
		mInputUploadBufferB);

	// Create the buffer that will be a UAV.
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mOutputBuffer)));

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
}
