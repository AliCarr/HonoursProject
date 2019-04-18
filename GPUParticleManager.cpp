#include "GPUParticleManager.h"

GPUParticleManager::GPUParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, std::unique_ptr<MeshGeometry>& mesh, ComPtr<ID3D12DescriptorHeap> mComputeHeap, ComPtr<ID3DBlob> mcsByteCode, ComPtr<ID3D12PipelineState > &mPSO)
{
	md3ddevice = device;
	CreateRootSignatures();
	GenerateParticleMesh(device, commandList);

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
			data->energy = 1;
		particleInputeData.push_back(std::move(*data));

		
	}
	frameIndex = 0;
	lastFrameIndex = 0;

	BuildResources();
	list = commandList;
	BuildDescriptors(md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), mComputeHeap, commandList);
	BuildPSO(mcsByteCode, mPSO);
	BuildACObjects();
}


GPUParticleManager::~GPUParticleManager()
{
}

void GPUParticleManager::CreateBuffers(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
}

void GPUParticleManager::BuildPSO(ComPtr<ID3DBlob> mcsByteCode, ComPtr<ID3D12PipelineState > &mPSO)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePSO = {};
	computePSO.pRootSignature = mComputeRootSignature.Get();
	computePSO.CS =
	{
		reinterpret_cast<BYTE*>(mcsByteCode->GetBufferPointer()),
		mcsByteCode->GetBufferSize()
	};
	ThrowIfFailed(md3ddevice->CreateComputePipelineState(&computePSO, IID_PPV_ARGS(&mPSO)));
}

//Called after "true" random is initialised 
XMFLOAT3 GPUParticleManager::StartingVelocity()
{
	return XMFLOAT3{ ((float)(rand() % 300) - 150.0f) / 100.0f,
		((float)(rand() % 300) - 150.0f) / 100.0f,
		((float)(rand() % 300) - 150.0f) / 100.0f };
}

//Called after "true" random is initialised 
XMFLOAT3 GPUParticleManager::StartingPosition()
{
	return XMFLOAT3{ ((float)(rand() % 400)) / 500.0f,
		((float)(rand() % 400)) / 400.0f,
		((float)(rand() % 400)) / 500.0f };
}

void GPUParticleManager::CreateRootSignatures()
{

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER computeSlotRootParameter[4];

	CD3DX12_DESCRIPTOR_RANGE ranges[4];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); 
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); //Changes table number, but not element in table

	computeSlotRootParameter[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
	computeSlotRootParameter[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC computeRootSigDesc(4, computeSlotRootParameter,
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

void GPUParticleManager::Render(ComPtr<ID3D12DescriptorHeap> &heap)
{
	for (int c = 0; c < currentNumberOfParticles; c++)
	{
		list->IASetVertexBuffers(0, 1, &mParticles.at(c)->geo->VertexBufferView());

		list->IASetIndexBuffer(&mParticles.at(c)->geo->IndexBufferView());
		list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		list->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

		list->DrawIndexedInstanced(mParticles.at(c)->geo->DrawArgs["particle"].IndexCount,
			1,
			mParticles.at(c)->geo->DrawArgs["particle"].StartIndexLocation,
			mParticles.at(c)->geo->DrawArgs["particle"].BaseVertexLocation,
			0);
	}
}

void GPUParticleManager::Execute(ID3D12GraphicsCommandList* list, ComPtr<ID3D12PipelineState> pso, ComPtr<ID3D12RootSignature> rootSig, ComPtr<ID3D12DescriptorHeap> &heap, ComPtr<ID3D12Resource>&, ComPtr<ID3D12CommandQueue> graphicsQueue, ComPtr<ID3D12Resource>& drawBuffer, ComPtr<ID3D12PipelineState> pso2, ComPtr<ID3D12DescriptorHeap> &heap2)
{
	bool async = true;
	if (!async)
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
		mCommandList->SetDescriptorHeaps(1, descriptorHeaps);

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

		whichHandle = !whichHandle;
		list->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(pUavResource,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		list->SetPipelineState(pso.Get());
		list->SetComputeRootSignature(mComputeRootSignature.Get());

		//Set heaps
		ID3D12DescriptorHeap* ppHeaps[] = { heap.Get() };
		list->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		m_srvUavDescriptorSize = md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap->GetGPUDescriptorHandleForHeapStart(), SRV, m_srvUavDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(heap->GetGPUDescriptorHandleForHeapStart(), UAV, m_srvUavDescriptorSize);

		//set compute root descriptor table
		list->SetComputeRootDescriptorTable(0U, srvHandle);
		list->SetComputeRootDescriptorTable(2U, uavHandle);

		
		list->Dispatch(static_cast<int>(ceil( numberOfParticles/ 64)), 1, 1);

		list->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(pUavResource,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_COPY_SOURCE));
	}

	if (async)
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
		mCommandList->SetDescriptorHeaps(1, descriptorHeaps);

		ID3D12CommandAllocator* pCommandAllocator = m_computeAllocators[frameIndex].Get();
		ID3D12GraphicsCommandList* pCommandList = m_computeCommandLists[frameIndex].Get();

		// Prepare for the next frame.
		ThrowIfFailed(pCommandAllocator->Reset());
		ThrowIfFailed(pCommandList->Reset(pCommandAllocator, pso.Get()));
		m_computeCommandQueue->Wait(m_graphicsCopyFences[lastFrameIndex].Get(), m_graphicsCopyFenceValues[lastFrameIndex]);
		RecordComputeTasks(pso, heap);

		whichHandle = !whichHandle;

		ThrowIfFailed(pCommandList->Close());

		ID3D12CommandList* ppCommandList[] = { m_computeCommandLists[frameIndex].Get() };

		m_computeCommandQueue->ExecuteCommandLists(1, ppCommandList);
		m_computeFenceValues[frameIndex] = m_computeFenceValue;
		m_computeCommandQueue->Signal(m_computeFences[frameIndex].Get(), m_computeFenceValue);

		++m_computeFenceValue;

		RecordCopyTasks(drawBuffer, pso2);

		ppCommandList[0] = { m_graphicsCopyCommandLists[frameIndex].Get() };

		graphicsQueue->Wait(m_computeFences[frameIndex].Get(), m_computeFenceValues[frameIndex]);

		graphicsQueue->ExecuteCommandLists(1, ppCommandList);

		m_graphicsCopyFenceValues[frameIndex] = m_graphicsCopyFenceValue;
		graphicsQueue->Signal(m_graphicsCopyFences[frameIndex].Get(), m_graphicsCopyFenceValue);

		++m_graphicsCopyFenceValue;
		RecordRenderTasks(pso2, heap2);

		ppCommandList[0] = { m_graphicsCommandLists[frameIndex].Get() };
		graphicsQueue->ExecuteCommandLists(1, ppCommandList);

		m_graphicsFenceValues[frameIndex] = m_graphicsFenceValue;
		graphicsQueue->Signal(m_graphicsFences[frameIndex].Get(), m_graphicsFenceValue);

		++m_graphicsFenceValue;
	}
}

void GPUParticleManager::BuildResources()
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
	
	md3ddevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&inputParticleBuffer2));

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

	//Set names for debugging
	inputParticleBuffer->SetName(L"Input Particle Buffer");
	outputParticleBuffer->SetName(L"Output Particle Buffer");
	uploadParticleBuffer->SetName(L"Upload Particle Buffer");
}

void GPUParticleManager::BuildDescriptors(UINT descriptorSize, ComPtr<ID3D12DescriptorHeap> &heap, ID3D12GraphicsCommandList* list)
{
	D3D12_SUBRESOURCE_DATA particleDataSub = {};
		particleDataSub.pData = reinterpret_cast<UINT8*>(&particleInputeData[0]);
		particleDataSub.RowPitch = numberOfParticles * sizeof(ComputeData);
		particleDataSub.SlicePitch = sizeof(particleDataSub);

	UpdateSubresources<1>(list, inputParticleBuffer.Get(), uploadParticleBuffer.Get(), 0, 0, 1, &particleDataSub);
	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputParticleBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	m_srvUavDescriptorSize = md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ThrowIfFailed(md3ddevice->GetDeviceRemovedReason());
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
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(heap->GetCPUDescriptorHandleForHeapStart(), 2U, m_srvUavDescriptorSize);	
		md3ddevice->CreateUnorderedAccessView(outputParticleBuffer.Get(), nullptr, &uavDesc, uavHandle);
		md3ddevice->CreateUnorderedAccessView(inputParticleBuffer.Get(), nullptr, &uavDesc, uavHandle.Offset(m_srvUavDescriptorSize));
}

//Only has to be called once, is then used as the template for each particle
bool GPUParticleManager::GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList)
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

void GPUParticleManager::UpdatePosition(int current, float time, UploadBuffer<Vertex>* buffer)
{
	if (mParticles.at(current)->accelertaion <= 1)
		mParticles.at(current)->accelertaion += time / 30.0f;

	for (UINT i = 0; i < vertexOffset; i++)
	{
		mParticles.at(current)->position.x += vert[i].Pos.x + (mParticles.at(current)->velocity.x*(time / 1));
		mParticles.at(current)->position.y += vert[i].Pos.y + (mParticles.at(current)->velocity.y*(time / 1)) - mParticles.at(current)->accelertaion;
		mParticles.at(current)->position.z += vert[i].Pos.z + (mParticles.at(current)->velocity.z*(time / 1));
		mParticles.at(current)->velocity.x = 0.00f;
		mParticles.at(current)->velocity.z = 0.00f;
	}

}

void GPUParticleManager::BuildACObjects()
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


	//Timing objects
	D3D12_RESOURCE_DESC cpuTimingBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64));

	for (int i = 0; i < FrameCount; ++i) 
	{
		md3ddevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&cpuTimingBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_timeQueryReadbackBuffer[i]));
	}

	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
		queryHeapDesc.Count = FrameCount; 
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

	md3ddevice->CreateQueryHeap(&queryHeapDesc,
		IID_PPV_ARGS(&m_timeQueryHeap));

	//Set all fence values to the starting values (zero)
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
}

void GPUParticleManager::RecordComputeTasks(ComPtr<ID3D12PipelineState>pso, ComPtr<ID3D12DescriptorHeap>&heap)
{
	ID3D12GraphicsCommandList* pCommandList = m_computeCommandLists[frameIndex].Get();

	for (int c = 0; c < 1; c++)
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

		pCommandList->SetPipelineState(pso.Get());
		pCommandList->SetComputeRootSignature(mComputeRootSignature.Get());

		//Set heaps
		ID3D12DescriptorHeap* ppHeaps[] = { heap.Get() };
		pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		m_srvUavDescriptorSize = md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap->GetGPUDescriptorHandleForHeapStart(), SRV, m_srvUavDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(heap->GetGPUDescriptorHandleForHeapStart(), UAV, m_srvUavDescriptorSize);

		//set compute root descriptor table
		pCommandList->SetComputeRootDescriptorTable(0U, srvHandle);
		pCommandList->SetComputeRootDescriptorTable(2U, uavHandle);

		pCommandList->Dispatch(static_cast<int>(ceil(numberOfParticles / 64)), 1, 1);

		pCommandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(pUavResource,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_COPY_SOURCE));

	}
}

void GPUParticleManager::RecordCopyTasks(ComPtr<ID3D12Resource>& drawBuffer, ComPtr<ID3D12PipelineState> pso)
{
	ThrowIfFailed(m_graphicsCopyAllocators[frameIndex]->Reset());
	ThrowIfFailed(m_graphicsCopyCommandLists[frameIndex]->Reset(m_graphicsCopyAllocators[frameIndex].Get(), pso.Get()));

	ID3D12GraphicsCommandList* commandList = m_graphicsCopyCommandLists[frameIndex].Get();

	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		pUavResource,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	commandList->ResourceBarrier(1, barriers);
	commandList->CopyResource(drawBuffer.Get(), pUavResource);

	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		pUavResource,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		drawBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);

	commandList->ResourceBarrier(2, barriers);
	ThrowIfFailed(commandList->Close());
}
void GPUParticleManager::RecordRenderTasks(ComPtr<ID3D12PipelineState> pso, ComPtr<ID3D12DescriptorHeap> &heap)
{
	ThrowIfFailed(m_graphicsAllocators[frameIndex]->Reset());

	ThrowIfFailed(m_graphicsCommandLists[frameIndex]->Reset(m_graphicsAllocators[frameIndex].Get(), pso.Get()));

	ID3D12GraphicsCommandList* commandList = m_graphicsCommandLists[frameIndex].Get();

	ID3D12DescriptorHeap* descriptorHeaps[] = { heap.Get() };
	m_graphicsCommandLists[frameIndex]->SetDescriptorHeaps(1, descriptorHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap->GetGPUDescriptorHandleForHeapStart(), 1U, md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_graphicsCommandLists[frameIndex]->SetGraphicsRootDescriptorTable(1, srvHandle);

	for (int c = 0; c < currentNumberOfParticles; c++)
	{
		m_graphicsCommandLists[frameIndex]->IASetVertexBuffers(0, 1, &mParticles.at(c)->geo->VertexBufferView());

		m_graphicsCommandLists[frameIndex]->IASetIndexBuffer(&mParticles.at(c)->geo->IndexBufferView());
		m_graphicsCommandLists[frameIndex]->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_graphicsCommandLists[frameIndex]->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

		m_graphicsCommandLists[frameIndex]->DrawIndexedInstanced(mParticles.at(c)->geo->DrawArgs["particle"].IndexCount,
			1,
			mParticles.at(c)->geo->DrawArgs["particle"].StartIndexLocation,
			mParticles.at(c)->geo->DrawArgs["particle"].BaseVertexLocation,
			0);
	}

}