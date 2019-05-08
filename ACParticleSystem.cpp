#include "ACParticleSystem.h"

namespace 
{
	void WaitForFence(ID3D12Fence* fence, UINT64 fenceValue, HANDLE fenceEvent)
	{
		if (fence->GetCompletedValue() < fenceValue) {
			ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
	}
}

ACParticleSystem::ACParticleSystem(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, ComPtr<ID3D12CommandQueue> &graphicsQueue, UI *userInter)
{
	md3ddevice = device;
	BuildRootSignatures();
	GenerateParticleMesh(device, commandList);

	mUI = new UI(*userInter);

	ZeroMemory(m_computeFenceValues, sizeof(m_computeFenceValues));
	ZeroMemory(m_graphicsFenceValues, sizeof(m_graphicsFenceValues));
	ZeroMemory(m_graphicsCopyFenceValues, sizeof(m_graphicsCopyFenceValues));

	m_computeFenceValue = 1;
	m_graphicsFenceValue = 1;
	m_graphicsCopyFenceValue = 1;

	amountOfComputeWork = 1;
	//Random offsets need to be as random as possible to prevent grid effect
	srand((unsigned)time(&mTime));

	for (int c = 0; c < currentNumberOfParticles; c++)
	{
		auto par = new ParticleInfromation();
		par->geo = new MeshGeometry(*mGeo);
		par->position = StartingPosition();
		par->dynamicVB = std::make_unique<UploadBuffer<Vertex>>(device.Get(), indexCount, false);
		par->velocity = StartingVelocity();
		par->accelertaion = 9.8f;
		par->energy = ((float)(rand() % 300) + 100.0f) / 100.0f;

		for (UINT i = 0; i < vertexOffset; i++)
		{
			//Offset the cooridnates for each vertex
			par->position.x += vert[i].Pos.x + 0.01;
			par->position.y += vert[i].Pos.y + 0.01;
			par->position.z += vert[i].Pos.z + 0.01;

			Vertex v;
			v.Pos = par->position;
			v.Color = XMFLOAT4(DirectX::Colors::White);
			v.texCoord = { 0.0f, 0.0f };
			v.id = c;
			par->dynamicVB->CopyData(i, v);

		}
		par->geo->VertexBufferGPU = par->dynamicVB->Resource();

		mParticles.push_back(std::move(par));

		//Need the particle information in our compute data vector
		auto data = new ComputeData();
		data->acceleration = XMFLOAT3(0, 0, 0);
		data->position = par->position;
		data->velocity = par->velocity;
		data->initialVelocity = par->velocity;
		data->energy = par->energy;
		particleInputeData.push_back(std::move(*data));
		

	}
	mGraphicsQueue = graphicsQueue;

	frameIndex = 0;
	lastFrameIndex = 0;
	BuildHeaps();
	BuildResources();
	BuildDescriptors(md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), computeHeap, commandList);
	BuildPSOs();
	BuildConstantBuffers();
	BuildACObjects();
}

ACParticleSystem::~ACParticleSystem()
{
	for (int i = 0; i < FrameCount; ++i) 
	{
			WaitForFence(m_computeFences[i].Get(),
				m_computeFenceValues[i], m_computeFenceEvents[i]);
			WaitForFence(m_graphicsFences[i].Get(),
				m_graphicsFenceValues[i], m_graphicsFenceEvents[i]);
			WaitForFence(m_graphicsCopyFences[i].Get(),
				m_graphicsCopyFenceValues[i], m_graphicsCopyFenceEvents[i]);
	
		WaitForFence(m_frameFences[i].Get(),
			m_frameFenceValues[i], m_frameFenceEvents[i]);
	}

	// Close handles to fence events.
	for (int i = 0; i < FrameCount; ++i) 
	{
		CloseHandle(m_computeFenceEvents[i]);
		CloseHandle(m_graphicsFenceEvents[i]);
		CloseHandle(m_graphicsCopyFenceEvents[i]);
		CloseHandle(m_frameFenceEvents[i]);
	}

}

//Called after "true" random is initialised 
XMFLOAT3 ACParticleSystem::StartingVelocity()
{
	return XMFLOAT3{ ((float)(rand() % 300) - 150.0f) / 100.0f,
		((float)(rand() % 300) - 150.0f) / 100.0f,
		((float)(rand() % 300) - 150.0f) / 100.0f };
}

//Called after "true" random is initialised 
XMFLOAT3 ACParticleSystem::StartingPosition()
{
	return XMFLOAT3{ ((float)(rand() % 400)) / 500.0f,
		((float)(rand() % 400)) / 400.0f,
		((float)(rand() % 400)) / 500.0f };
}

void ACParticleSystem::Update(ObjectConstants constants, int num, int work)
{
	mObjectCB->CopyData(0, constants);
	currentNumberOfParticles = num;
	amountOfComputeWork = work;
}

void ACParticleSystem::Execute(ComPtr<ID3D12CommandQueue> graphicsQueue,
							   ComPtr<ID3D12Resource>& drawBuffer, 
							   ComPtr<IDXGISwapChain> &mSwapChain, 
							   D3D12_RESOURCE_BARRIER &bar,
							   D3D12_RESOURCE_BARRIER& bar2, 
							   D3D12_CPU_DESCRIPTOR_HANDLE &backView, 
							   D3D12_CPU_DESCRIPTOR_HANDLE &depthView, 
							   D3D12_VIEWPORT &viewPort, 
							   D3D12_RECT &rect, 
							   UI* ui)
{
		/////COMPUTE PHASE//////////////////
		m_computeCommandQueue->Wait(m_graphicsCopyFences[lastFrameIndex].Get(), m_graphicsCopyFenceValues[lastFrameIndex]);
		RecordComputeTasks();
		ID3D12CommandList* ppCommandList[] = { m_computeCommandLists[frameIndex].Get() };
		m_computeCommandQueue->ExecuteCommandLists(1, ppCommandList);
		m_computeFenceValues[frameIndex] = m_computeFenceValue;
		m_computeCommandQueue->Signal(m_computeFences[frameIndex].Get(), m_computeFenceValue);
		++m_computeFenceValue;

		/////COPY PHASE/////////////
		RecordCopyTasks(drawBuffer);
		ppCommandList[0] = { m_graphicsCopyCommandLists[frameIndex].Get() };
		mGraphicsQueue->Wait(m_computeFences[frameIndex].Get(), m_computeFenceValues[frameIndex]);
		mGraphicsQueue->ExecuteCommandLists(1, ppCommandList);
		m_graphicsCopyFenceValues[frameIndex] = m_graphicsCopyFenceValue;
		mGraphicsQueue->Signal(m_graphicsCopyFences[frameIndex].Get(), m_graphicsCopyFenceValue);
		++m_graphicsCopyFenceValue;

		////////RENDER PHASE////////////////
		RecordRenderTasks(mSwapChain, bar, bar2, backView, depthView, viewPort, rect, ui);
		ppCommandList[0] = { m_graphicsCommandLists[frameIndex].Get() };
		mGraphicsQueue->ExecuteCommandLists(1, ppCommandList);
		m_graphicsFenceValues[frameIndex] = m_graphicsFenceValue;
		mGraphicsQueue->Signal(m_graphicsFences[frameIndex].Get(), m_graphicsFenceValue);
		++m_graphicsFenceValue;

		// Assign the current fence value to the current frame.
		m_frameFenceValues[frameIndex] = m_frameFenceValue;

		// Signal and increment the fence value.
		ThrowIfFailed(mGraphicsQueue->Signal(m_frameFences[frameIndex].Get(), m_frameFenceValue));
		++m_frameFenceValue;

		// Update the frame index.
		lastFrameIndex = frameIndex;
		frameIndex += 1;

		if (frameIndex >= 4)
			frameIndex = 0;
		// If the next frame is not ready to be rendered yet, wait until it is ready.
		WaitForFence(m_frameFences[frameIndex].Get(),
			m_frameFenceValues[frameIndex], m_frameFenceEvents[frameIndex]);

		assert(mSwapChain);

		ThrowIfFailed(mSwapChain->Present(0, 0));
}

void ACParticleSystem::BuildResources()
{
	D3D12_RESOURCE_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(D3D12_RESOURCE_DESC));
		bufferDesc.Alignment = 0;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		bufferDesc.Width = numberOfParticles * sizeof(ComputeData);
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; //used for buffers as texture data can be located in them without creating a texture object
		bufferDesc.SampleDesc.Count = 1;

	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfParticles * sizeof(ComputeData));

	//Create the input buffer using the descriptor above
	md3ddevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&inputParticleBuffer));

	//create the output buffer using the desc above
	md3ddevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&outputParticleBuffer));

	//Create the upload buffer that will handle the data being copied to the above buffers
	md3ddevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadParticleBuffer));

	ThrowIfFailed(md3ddevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadParticleBuffer2)));

	//Set names for debugging
	inputParticleBuffer->SetName(L"Input Particle Buffer");
	outputParticleBuffer->SetName(L"Output Particle Buffer");
	uploadParticleBuffer->SetName(L"Upload Particle Buffer");
}

void ACParticleSystem::BuildDescriptors(UINT descriptorSize, ComPtr<ID3D12DescriptorHeap> &heap, ID3D12GraphicsCommandList* list)
{
	D3D12_SUBRESOURCE_DATA particleDataSub = {};
		particleDataSub.pData = reinterpret_cast<UINT8*>(&particleInputeData[0]);
		particleDataSub.RowPitch = numberOfParticles * sizeof(ComputeData);
		particleDataSub.SlicePitch = sizeof(particleDataSub);

	UpdateSubresources<1>(list, inputParticleBuffer.Get(), uploadParticleBuffer.Get(), 0, 0, 1, &particleDataSub);
	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputParticleBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	m_srvUavDescriptorSize = md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numberOfParticles;
		srvDesc.Buffer.StructureByteStride = sizeof(ComputeData);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(heap->GetCPUDescriptorHandleForHeapStart(), SRV, m_srvUavDescriptorSize);
		md3ddevice->CreateShaderResourceView(inputParticleBuffer.Get(), &srvDesc, srvHandle);
		md3ddevice->CreateShaderResourceView(outputParticleBuffer.Get(), &srvDesc, srvHandle.Offset(m_srvUavDescriptorSize));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numberOfParticles;
		uavDesc.Buffer.StructureByteStride = sizeof(ComputeData);
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(heap->GetCPUDescriptorHandleForHeapStart(), UAV, m_srvUavDescriptorSize);
		md3ddevice->CreateUnorderedAccessView(outputParticleBuffer.Get(), nullptr, &uavDesc, uavHandle);
		md3ddevice->CreateUnorderedAccessView(inputParticleBuffer.Get(), nullptr, &uavDesc, uavHandle.Offset(m_srvUavDescriptorSize));
}

//Only has to be called once, is then used as the template for each particle
bool ACParticleSystem::GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList)
{
	mGeo = new MeshGeometry();
	mGeo->Name = "shapeGeo";

	GeometryGenerator::MeshData particleMeshData = generator.CreateGrid(width, depth, rows, columns);

	vertexOffset = (UINT)particleMeshData.Vertices.size();
	indexOffset = (UINT)particleMeshData.Indices32.size();
	indexCount = (UINT)particleMeshData.Indices32.size();

	boxSubmesh.IndexCount = (UINT)particleMeshData.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;

	totalVertexCount = vertexOffset;

	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;


	for (size_t i = 0; i < vertexOffset; i++)
	{
		vertices[i].Pos = particleMeshData.Vertices[i].Position;
		vertices[i].Color = XMFLOAT4(DirectX::Colors::Crimson);

		vert[i].Pos = vertices[i].Pos;
	}

	if (vertices.size() == 0)
		return false;

	indices.insert(indices.end(),
		std::begin(particleMeshData.GetIndices16()),
		std::end(particleMeshData.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mGeo->VertexBufferCPU = nullptr;
	mGeo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeo->IndexBufferCPU));
	CopyMemory(mGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList,
		indices.data(),
		ibByteSize,
		mGeo->IndexBufferUploader);

	mGeo->VertexByteStride = sizeof(Vertex);
	mGeo->VertexBufferByteSize = vbByteSize;
	mGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mGeo->IndexBufferByteSize = ibByteSize;
	mGeo->DrawArgs["particle"] = boxSubmesh;

	if (mGeo == NULL)
		return false;

	return true;
}

void ACParticleSystem::BuildACObjects()
{
	//Render and copy objects (queue will just be the normal one)
	for (int i = 0; i < FrameCount; ++i)
	{
		//create all four graphics lists
		ThrowIfFailed(md3ddevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_graphicsAllocators[i])));
		ThrowIfFailed(md3ddevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsAllocators[i].Get(), nullptr, IID_PPV_ARGS(&m_graphicsCommandLists[i])));
		ThrowIfFailed(md3ddevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_graphicsFences[i])));

		m_graphicsCommandLists[i]->Close();

		m_graphicsCommandLists[i].Get()->SetName(L"AC Graphics Command Lists");

		//Create all four copy lists
		ThrowIfFailed(md3ddevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_graphicsCopyAllocators[i])));
		ThrowIfFailed(md3ddevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsCopyAllocators[i].Get(), nullptr, IID_PPV_ARGS(&m_graphicsCopyCommandLists[i])));
		ThrowIfFailed(md3ddevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_graphicsCopyFences[i])));

		m_graphicsCopyCommandLists[i]->Close();

		m_graphicsCopyCommandLists[i].Get()->SetName(L"Copy Command Lists");
	}

	//Compute objects
	//Command queue for the compute work, copy work goes into render queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
	ThrowIfFailed(md3ddevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_computeCommandQueue)));

	for (int i = 0; i < FrameCount; ++i)
	{
		//create all four command lists
		ThrowIfFailed(md3ddevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_computeAllocators[i])));
		ThrowIfFailed(md3ddevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeAllocators[i].Get(), nullptr, IID_PPV_ARGS(&m_computeCommandLists[i])));
		ThrowIfFailed(md3ddevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_computeFences[i])));

		m_computeCommandLists[i]->Close();

		m_computeCommandLists[i].Get()->SetName(L"Compute Command Lists");
	}

	m_frameFenceValue = 0;
	for (int i = 0; i < FrameCount; ++i) {
		m_frameFenceValues[i] = m_frameFenceValue;
		md3ddevice->CreateFence(m_frameFenceValues[i], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_frameFences[i]));
		m_frameFenceEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		m_graphicsFenceValues[i] = 0;
		ThrowIfFailed(md3ddevice->CreateFence(m_graphicsFenceValues[i], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_graphicsFences[i])));
		m_graphicsFenceEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		m_graphicsCopyFenceValues[i] = 0;
		ThrowIfFailed(md3ddevice->CreateFence(m_graphicsCopyFenceValues[i], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_graphicsCopyFences[i])));
		m_graphicsCopyFenceEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		m_computeFenceValues[i] = 0;
		ThrowIfFailed(md3ddevice->CreateFence(m_computeFenceValues[i], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_computeFences[i])));
		m_computeFenceEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	m_uploadFenceValue = 0;
	ThrowIfFailed(md3ddevice->CreateFence(m_uploadFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_uploadFence)));

	m_uploadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	mGraphicsQueue->Signal(m_uploadFence.Get(), 1);
	WaitForFence(m_uploadFence.Get(), 1, m_uploadEvent);
}

void ACParticleSystem::RecordComputeTasks()
{
	ID3D12CommandAllocator* pCommandAllocator = m_computeAllocators[frameIndex].Get();
	ID3D12GraphicsCommandList* pCommandList = m_computeCommandLists[frameIndex].Get();

	ThrowIfFailed(pCommandAllocator->Reset());
	ThrowIfFailed(pCommandList->Reset(pCommandAllocator, computePso.Get()));

	for (int c = 0; c < amountOfComputeWork; c++)
	{
		if (whichHandle == true)
		{
			SRV = 1U;
			UAV = 3U;
			pUavResource = inputParticleBuffer.Get();
		}
		else
		{
			SRV = 0U;
			UAV = 2U;
			pUavResource = outputParticleBuffer.Get();
		}

		pCommandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(pUavResource,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		pCommandList->SetPipelineState(computePso.Get());
		pCommandList->SetComputeRootSignature(mComputeRootSignature.Get());

		//Set heaps
		ID3D12DescriptorHeap* ppHeaps[] = { computeHeap.Get() };
		pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		m_srvUavDescriptorSize = md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(computeHeap->GetGPUDescriptorHandleForHeapStart(), SRV, m_srvUavDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(computeHeap->GetGPUDescriptorHandleForHeapStart(), UAV, m_srvUavDescriptorSize);

		UINT test = 1;
		//set compute root descriptor table
		pCommandList->SetComputeRootDescriptorTable(0U, srvHandle);
		pCommandList->SetComputeRootDescriptorTable(2U, uavHandle);
		pCommandList->SetComputeRoot32BitConstants(3U, sizeof(float), &test, 0);

		pCommandList->Dispatch(static_cast<int>(ceil(currentNumberOfParticles / 32)), 1, 1);

		pCommandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(pUavResource,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	}

	whichHandle = !whichHandle;
	ThrowIfFailed(pCommandList->Close());
}

void ACParticleSystem::RecordCopyTasks(ComPtr<ID3D12Resource>& drawBuffer)
{
	ThrowIfFailed(m_graphicsCopyAllocators[frameIndex]->Reset());
	ThrowIfFailed(m_graphicsCopyCommandLists[frameIndex]->Reset(m_graphicsCopyAllocators[frameIndex].Get(), pso.Get()));
	
	ID3D12GraphicsCommandList* pCommandList = m_graphicsCopyCommandLists[frameIndex].Get();
	ID3D12Resource* particleBuffer = pUavResource;

	D3D12_RESOURCE_BARRIER barriers[2];

	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		particleBuffer,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	pCommandList->ResourceBarrier(1, barriers);
	pCommandList->CopyResource(m_particleBufferForDraw.Get(), pUavResource);

	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		particleBuffer,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_particleBufferForDraw.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	pCommandList->ResourceBarrier(2, barriers);

	ThrowIfFailed(pCommandList->Close());
}
void ACParticleSystem::RecordRenderTasks(ComPtr<IDXGISwapChain> &chain, D3D12_RESOURCE_BARRIER &bar, D3D12_RESOURCE_BARRIER& bar2, D3D12_CPU_DESCRIPTOR_HANDLE &backView, D3D12_CPU_DESCRIPTOR_HANDLE &depthView, D3D12_VIEWPORT &viewPort, D3D12_RECT &rect, UI* ui)
{
	ThrowIfFailed(m_graphicsAllocators[frameIndex]->Reset());

	ThrowIfFailed(m_graphicsCommandLists[frameIndex]->Reset(m_graphicsAllocators[frameIndex].Get(), pso.Get()));

	ID3D12GraphicsCommandList* commandList = m_graphicsCommandLists[frameIndex].Get();


	commandList->RSSetViewports(1, &viewPort);
	commandList->RSSetScissorRects(1, &rect);

	commandList->ResourceBarrier(1, &bar);
	commandList->ClearRenderTargetView(backView, ui->GetColour(), 0, nullptr);
	commandList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	commandList->OMSetRenderTargets(1, &backView, true, &depthView);

	commandList->SetPipelineState(pso.Get());
	commandList->SetGraphicsRootSignature(renderSig.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap.Get() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(cbvHeap->GetGPUDescriptorHandleForHeapStart(), 1U, md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	commandList->SetGraphicsRootDescriptorTable(1U, srvHandle);

	for (int c = 0; c < currentNumberOfParticles; c++)
	{
		commandList->IASetVertexBuffers(0, 1, &mParticles.at(c)->geo->VertexBufferView());

		commandList->IASetIndexBuffer(&mParticles.at(c)->geo->IndexBufferView());
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetGraphicsRootDescriptorTable(0, cbvHeap->GetGPUDescriptorHandleForHeapStart());

		commandList->DrawIndexedInstanced(mParticles.at(c)->geo->DrawArgs["particle"].IndexCount,
			1,
			mParticles.at(c)->geo->DrawArgs["particle"].StartIndexLocation,
			mParticles.at(c)->geo->DrawArgs["particle"].BaseVertexLocation,
			0);
	}

	D3D12_RESOURCE_BARRIER barriers[1];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_particleBufferForDraw.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST);

	ui->GUIRender(commandList);

	commandList->ResourceBarrier(1, barriers);
	ThrowIfFailed(commandList->Close());
}

void ACParticleSystem::BuildHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC computeHeapDesc = {};
		computeHeapDesc.NumDescriptors = 4U;
		computeHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		computeHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3ddevice->CreateDescriptorHeap(&computeHeapDesc, IID_PPV_ARGS(&computeHeap)));


	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 13; //Originally 11, we're adding 2 more
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3ddevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));
}

void ACParticleSystem::BuildPSOs()
{
		mcsByteCode = d3dUtil::CompileShader(L"Shaders\\particleCS.hlsl", nullptr, "UpdateWavesCS", "cs_5_0");

		D3D12_COMPUTE_PIPELINE_STATE_DESC computePSO = {};
		computePSO.pRootSignature = mComputeRootSignature.Get();
		computePSO.CS =
		{
			reinterpret_cast<BYTE*>(mcsByteCode->GetBufferPointer()),
			mcsByteCode->GetBufferSize()
		};
		ThrowIfFailed(md3ddevice->CreateComputePipelineState(&computePSO, IID_PPV_ARGS(&computePso)));

		mvsByteCode = d3dUtil::CompileShader(L"Shaders\\colorVS.hlsl", nullptr, "VS", "vs_5_0");
		mpsByteCode = d3dUtil::CompileShader(L"Shaders\\colorPS.hlsl", nullptr, "PS", "ps_5_0");

		mInputLayout =
		{
			//Semantic Name, Semantic Index, Format, Input slot, Aligned Byte Offset, Input Slot Class, Instance Data Step Rate
			{ "POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,   0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEX",      0,       DXGI_FORMAT_R32G32_FLOAT, 0,  12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "ID",		  0,		   DXGI_FORMAT_R16_UINT, 0,  36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};


		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
			ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
			psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
			psoDesc.pRootSignature = renderSig.Get();
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
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			psoDesc.SampleDesc.Quality = 0;
			psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		ThrowIfFailed(md3ddevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

void ACParticleSystem::BuildRootSignatures()
{
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE range[1];
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	slotRootParameter[1].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_VERTEX);

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

	ThrowIfFailed(md3ddevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&renderSig)));

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER computeSlotRootParameter[5];

	CD3DX12_DESCRIPTOR_RANGE ranges[4];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); //Changes table number, but not element in table

	computeSlotRootParameter[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[4].InitAsConstants(1, 0); //register 1b

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC computeRootSigDesc(5, computeSlotRootParameter,
		0, nullptr);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> computeSerializedRootSig = nullptr;
	ComPtr<ID3DBlob> computeErrorBlob = nullptr;
	HRESULT computehr = D3D12SerializeRootSignature(&computeRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		computeSerializedRootSig.GetAddressOf(), computeErrorBlob.GetAddressOf());

	if (computeErrorBlob != nullptr)
	{
		::OutputDebugStringA((char*)computeErrorBlob->GetBufferPointer());
	}

	ThrowIfFailed(md3ddevice->CreateRootSignature(
		0,
		computeSerializedRootSig->GetBufferPointer(),
		computeSerializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mComputeRootSignature.GetAddressOf())));

	mComputeRootSignature->SetName(L"Compute Root Signature");
}

void ACParticleSystem::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3ddevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	// Offset to the ith object constant buffer in the buffer.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		md3ddevice->CreateConstantBufferView(
			&cbvDesc,
			cbvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numberOfParticles;
		srvDesc.Buffer.StructureByteStride = sizeof(ComputeData);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	D3D12_RESOURCE_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(D3D12_RESOURCE_DESC));
		bufferDesc.Alignment = 0;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		bufferDesc.Width = numberOfParticles * sizeof(ComputeData);
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; //used for buffers as texture data can be located in them without creating a texture object
		bufferDesc.SampleDesc.Count = 1;

	md3ddevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_particleBufferForDraw));

	m_particleBufferForDraw->SetName(L"Draw Buffer");

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(cbvHeap->GetCPUDescriptorHandleForHeapStart(), 1U, md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	md3ddevice->CreateShaderResourceView(m_particleBufferForDraw.Get(), &srvDesc, srvHandle);
}