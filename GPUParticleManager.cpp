#include "GPUParticleManager.h"

GPUParticleManager::GPUParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, ComPtr<ID3DBlob> mcsByteCode, ComPtr<ID3D12PipelineState > &mPSO)
{
	md3ddevice = device;
	CreateRootSignatures();
	GenerateParticleMesh(device, commandList);
	currentAmountOfComputeWork = 1;
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
			data->energy = par->energy = ((float)(rand() % 100)) / 100.0f;
		particleInputeData.push_back(std::move(*data));
	}

	BuildResources();
	list = commandList;
	BuildDescriptors(md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), mComputeHeap, commandList);
	BuildPSO(mcsByteCode, mPSO);
}


GPUParticleManager::~GPUParticleManager()
{
}

void GPUParticleManager::CreateBuffers(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
}

void GPUParticleManager::BuildPSO(ComPtr<ID3DBlob> mcsByteCode, ComPtr<ID3D12PipelineState> &mPSO)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePSO = {};
	computePSO.pRootSignature = mComputeRootSignature.Get();
	computePSO.CS =
	{
		reinterpret_cast<BYTE*>(mcsByteCode->GetBufferPointer()),
		mcsByteCode->GetBufferSize()
	};
	ThrowIfFailed(md3ddevice->CreateComputePipelineState(&computePSO, IID_PPV_ARGS(&pso)));


	//mvsByteCode = d3dUtil::CompileShader(L"Shaders\\colorVS.hlsl", nullptr, "VS", "vs_5_0");
	//mpsByteCode = d3dUtil::CompileShader(L"Shaders\\colorPS.hlsl", nullptr, "PS", "ps_5_0");
	//mInputLayout =
	//{
	//	//Semantic Name, Semantic Index, Format, Input slot, Aligned Byte Offset, Input Slot Class, Instance Data Step Rate
	//	{ "POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,   0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//{ "TEX",      0,       DXGI_FORMAT_R32G32_FLOAT, 0,  12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//{ "ID",		  0,		   DXGI_FORMAT_R16_UINT, 0,  36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	//};


	//D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	//ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	//psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	//psoDesc.pRootSignature = mRootSignature.Get();
	//psoDesc.VS =
	//{
	//	reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
	//	mvsByteCode->GetBufferSize()
	//};
	//psoDesc.PS =
	//{
	//	reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
	//	mpsByteCode->GetBufferSize()
	//};
	//psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	//psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//psoDesc.SampleMask = UINT_MAX;
	//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//psoDesc.NumRenderTargets = 1;
	//psoDesc.RTVFormats[0] = mBackBufferFormat;
	//psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	//psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	//psoDesc.DSVFormat = mDepthStencilFormat;
	//ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO["renderPSO"])));
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
	computeSlotRootParameter[4].InitAsConstants(5, 1);


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

void GPUParticleManager::Render(ComPtr<ID3D12DescriptorHeap> &heap)
{
	float testing = 10;
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap->GetGPUDescriptorHandleForHeapStart(), 1U, md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	
	list->SetGraphicsRootDescriptorTable(1, srvHandle);


	for (int c = 0; c < 3; c++)
	{
		list->IASetVertexBuffers(0, 1, &mParticles.at(c)->geo->VertexBufferView());

		list->IASetIndexBuffer(&mParticles.at(c)->geo->IndexBufferView());
		list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		list->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

		list->DrawIndexedInstanced(mParticles.at(c)->geo->DrawArgs["particle"].IndexCount,
			1000000,
			mParticles.at(c)->geo->DrawArgs["particle"].StartIndexLocation,
			mParticles.at(c)->geo->DrawArgs["particle"].BaseVertexLocation,
			0);
	}
}

void GPUParticleManager::Update(int num, int work)
{
	currentNumberOfParticles = num;
	currentAmountOfComputeWork = work;
}

void GPUParticleManager::Execute(UI &mUI)
{
	for (int c = 0; c < currentAmountOfComputeWork; c++)
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

		whichHandle = !whichHandle;
		list->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(pUavResource,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		list->SetPipelineState(pso.Get());
		list->SetComputeRootSignature(mComputeRootSignature.Get());

		//Set heaps
		ID3D12DescriptorHeap* ppHeaps[] = { mComputeHeap.Get() };
		list->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		m_srvUavDescriptorSize = md3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(mComputeHeap->GetGPUDescriptorHandleForHeapStart(), SRV, m_srvUavDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(mComputeHeap->GetGPUDescriptorHandleForHeapStart(), UAV, m_srvUavDescriptorSize);

		
		//set compute root descriptor table
		list->SetComputeRootDescriptorTable(0U, srvHandle);
		list->SetComputeRootDescriptorTable(2U, uavHandle);
		list->SetComputeRoot32BitConstant(3, mUI.GetParticleSize(), 0);
		list->SetComputeRoot32BitConstant(4, mUI.forceX,1);
		list->SetComputeRoot32BitConstant(3, 10000, 2);
		list->SetComputeRoot32BitConstant(3, mUI.forceZ,3);
		list->SetComputeRoot32BitConstant(3, 1, 4);



		list->Dispatch(static_cast<int>(ceil(numberOfParticles / 32)), 1, 1);

		list->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(pUavResource,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_COPY_SOURCE));
	}

	size = mUI.GetParticleSize();
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


	D3D12_DESCRIPTOR_HEAP_DESC computeHeapDesc = {};
		computeHeapDesc.NumDescriptors = 4U;
		computeHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		computeHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3ddevice->CreateDescriptorHeap(&computeHeapDesc, IID_PPV_ARGS(&mComputeHeap)));


	//Set names for debugging
	inputParticleBuffer->SetName(L"Input Particle Buffer");
	outputParticleBuffer->SetName(L"Output Particle Buffer");
	uploadParticleBuffer->SetName(L"Upload Particle Buffer");
	mComputeHeap->SetName(L"Compute Heap");
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mComputeHeap->GetCPUDescriptorHandleForHeapStart(), SRV, m_srvUavDescriptorSize);
		md3ddevice->CreateShaderResourceView(inputParticleBuffer.Get(), &srvDesc, srvHandle);	
		md3ddevice->CreateShaderResourceView(outputParticleBuffer.Get(), &srvDesc, srvHandle.Offset(m_srvUavDescriptorSize));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numberOfParticles;
		uavDesc.Buffer.StructureByteStride = sizeof(ComputeData);
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(mComputeHeap->GetCPUDescriptorHandleForHeapStart(), 2U, m_srvUavDescriptorSize);
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


