#include "GPUParticleManager.h"



GPUParticleManager::GPUParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	GenerateParticleMesh(device, commandList);

	//Random offsets need to be as random as possible to prevent grid effect
	srand((unsigned)time(&mTime));
	for (int c = 0; c < numberOfParticles; c++)
	{
		auto par = new ParticleInfromation();
			par->geo = new MeshGeometry(*mGeo);
			par->position = StartingPosition();
			par->dynamicVB = std::make_unique<UploadBuffer<Vertex>>(device.Get(), indexCount, false);
			par->velocity = StartingVelocity();
			par->accelertaion = 0;
			par->energy = ((float)(rand() % 300) + 100.0f) / 100.0f;
		mParticles.push_back(std::move(par));

		//Need the particle information in our compute data vector
		auto data = new ComputeData();
			data->initialPosition = par->position;
			data->position = par->position;
			data->velocity = par->velocity;
		particleInputeData.push_back(std::move(*data));
	}
	md3ddevice = device;
	BuildResources();
}


GPUParticleManager::~GPUParticleManager()
{
}

void GPUParticleManager::CreateBuffers(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
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

void GPUParticleManager::CreateRootSignatures(Microsoft::WRL::ComPtr<ID3D12Device> &device)
{
}

void GPUParticleManager::Execute(ID3D12GraphicsCommandList* list, ComPtr<ID3D12PipelineState> pso, ComPtr<ID3D12RootSignature> rootSig, ComPtr<ID3D12DescriptorHeap> &heap)
{
	//Let the output buffer be used as a UAV
	list->ResourceBarrier(1, 
						  &CD3DX12_RESOURCE_BARRIER::Transition(outputParticleBuffer.Get(), 
						  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
						  D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	
	list->SetPipelineState(pso.Get());
	list->SetComputeRootSignature(rootSig.Get());
	
	//Set heaps
	ID3D12DescriptorHeap* ppHeaps[] = { heap.Get() };
	list->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_srvUavDescriptorSize = md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap->GetGPUDescriptorHandleForHeapStart(), 0U, m_srvUavDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(heap->GetGPUDescriptorHandleForHeapStart(), 2U, m_srvUavDescriptorSize);

	//set compute root descriptor table
	list->SetComputeRootDescriptorTable(0U, srvHandle);
	list->SetComputeRootDescriptorTable(1U, srvHandle.Offset(m_srvUavDescriptorSize));
	list->SetComputeRootDescriptorTable(2U, uavHandle);

		list->Dispatch(numberOfParticles, 1, 1);

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

void GPUParticleManager::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
	UINT descriptorSize, ComPtr<ID3D12DescriptorHeap> &heap, ID3D12GraphicsCommandList* list)
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
		md3ddevice->CreateShaderResourceView(inputParticleBuffer2.Get(), &srvDesc, srvHandle.Offset(m_srvUavDescriptorSize));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numberOfParticles;
		uavDesc.Buffer.StructureByteStride = sizeof(ComputeData);
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(heap->GetCPUDescriptorHandleForHeapStart(), 2U, m_srvUavDescriptorSize);	
		md3ddevice->CreateUnorderedAccessView(outputParticleBuffer.Get(), nullptr, &uavDesc, uavHandle);
	
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