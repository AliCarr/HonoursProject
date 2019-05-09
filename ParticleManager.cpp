#include "ParticleManager.h"
#include <cstdlib>

ParticleManager::ParticleManager(ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	GenerateParticleMesh(device, commandList);

	//Enable better random
	srand((unsigned)time(&mTime));

	//Build the maximum numebr of particles 
	for (int c = 0; c < numberOfParticles; c++)
	{
		auto par = new ParticleInfromation();
			par->geo = new MeshGeometry(*mGeo);
			par->position = StartingPosition();
			par->dynamicVB = std::make_unique<UploadBuffer<Vertex>>(device.Get(), indexCount, false);
			par->velocity = StartingVelocity();
			par->accelertaion = 0;
			par->energy = ((float)(rand() % 300) + 100.0f) / 100.0f;
			par->geo->VertexBufferGPU = par->dynamicVB->Resource(); //Prevents null values when updating larger systems
		mParticles.push_back(std::move(par));
 	}

	md3ddevice = device;
	list = commandList;
	BuildHeap();
	BuildRootSignature();
	BuildPSO();
}

ParticleManager::~ParticleManager()
{
	for (int c = 0; c < numberOfParticles; c++)
	{
		if (mParticles.at(c))
		{
			delete mParticles.at(c);
			mParticles.at(c) = 0;
		}
	}

	if (mGeo)
	{
		delete mGeo;
		mGeo = 0;
	}
}

void ParticleManager::Update(float time, int num, DirectX::XMMATRIX mat, UI &mUi)
{
	currentNumberOfParticles = num;

	for (int c = 0; c < currentNumberOfParticles; c++)
	{
		auto currVB = mParticles.at(c)->dynamicVB.get();
			mParticles.at(c)->energy -= time;
			UpdatePosition(c, time, currVB, mUi);
			mParticles.at(c)->geo->VertexBufferGPU = currVB->Resource();
	}

	//The y position and pulse colour need to go
	ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.WorldViewProj, mat);
		objConstants.yPosiiton += time;
		objConstants.pulseColour = XMFLOAT4(1, 0, 0, 1);
	mObjectCB->CopyData(0, objConstants);

}

void ParticleManager::Render(ComPtr<ID3D12DescriptorHeap> &heap)
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	list->SetDescriptorHeaps(1, descriptorHeaps);
	list->SetPipelineState(pso.Get());
	list->SetGraphicsRootSignature(rootSig.Get());

	for (int c = 0; c < currentNumberOfParticles; c++)
	{
		list->IASetVertexBuffers(0, 1, &mParticles.at(c)->geo->VertexBufferView());

		list->IASetIndexBuffer(&mParticles.at(c)->geo->IndexBufferView());
		list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		list->SetGraphicsRootDescriptorTable(0U, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

		list->DrawIndexedInstanced(mParticles.at(c)->geo->DrawArgs["particle"].IndexCount,
										  1,
										  mParticles.at(c)->geo->DrawArgs["particle"].StartIndexLocation,
										  mParticles.at(c)->geo->DrawArgs["particle"].BaseVertexLocation,
										  0);
	}
}

XMFLOAT3 ParticleManager::StartingVelocity()
{
	return XMFLOAT3{ ((float)(rand() % 300) - 150.0f) / 100.0f,
					 ((float)(rand() % 300) - 150.0f) / 100.0f,
					 ((float)(rand() % 300) - 150.0f) / 100.0f };
}

XMFLOAT3 ParticleManager::StartingPosition()
{
	return XMFLOAT3{ ((float)(rand() % 400)) / 500.0f,
					 ((float)(rand() % 400)) / 400.0f,
					 ((float)(rand() % 400)) / 500.0f };
}

void ParticleManager::ParticleReset(int current)
{
	mParticles.at(current)->energy = 1;
	mParticles.at(current)->position = StartingPosition();
	mParticles.at(current)->accelertaion = 0;
	mParticles.at(current)->velocity = StartingVelocity();
}

bool ParticleManager::GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList)
{
	mGeo = new MeshGeometry();
	mGeo->Name = "shapeGeo";

	GeometryGenerator::MeshData particleMeshData = generator.CreateGrid(width, depth, rows, columns);

	vertexOffset = (UINT)particleMeshData.Vertices.size();
	indexOffset = (UINT)particleMeshData.Indices32.size();
	indexCount = (UINT)particleMeshData.Indices32.size();

	boxSubmesh.IndexCount = (UINT)particleMeshData.Indices32.size();
	boxSubmesh.StartIndexLocation =  0;
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

void ParticleManager::UpdatePosition(int current, float time, UploadBuffer<Vertex>* buffer, UI &mUi)
{
	float fixedDeltaTime = 0.0178f;
	if(mParticles.at(current)->accelertaion <= maxAcceleration)
		mParticles.at(current)->accelertaion += fixedDeltaTime / 30.0f;

	for (UINT i = 0; i < vertexOffset; i++)
	{
		mParticles.at(current)->position.y -= mParticles.at(current)->accelertaion / 30;

		//Offset the cooridnates for each vertex
		mParticles.at(current)->position.x += vert[i].Pos.x*mUi.GetParticleSize() + (mParticles.at(current)->velocity.x*(fixedDeltaTime)) ;
		mParticles.at(current)->position.y += vert[i].Pos.y + (mParticles.at(current)->velocity.y*(fixedDeltaTime)) ;
		mParticles.at(current)->position.z += vert[i].Pos.z*mUi.GetParticleSize() + (mParticles.at(current)->velocity.z*(fixedDeltaTime)) ;

		Vertex v;
			v.Pos = mParticles.at(current)->position;
			v.Color = (XMFLOAT4)mUi.GetParColour(); 
			v.texCoord = { 0.0f, 0.0f };
			v.id = current;
		buffer->CopyData(i, v);

		if (mParticles.at(current)->velocity.x <= 0.0018)
		{
			mParticles.at(current)->velocity.x += mParticles.at(current)->velocity.x/10000;
			mParticles.at(current)->velocity.y += mParticles.at(current)->velocity.y/10000;
			mParticles.at(current)->velocity.z += mParticles.at(current)->velocity.z/10000;
		}
	}
	if (mParticles.at(current)->accelertaion <= 0.1)
		mParticles.at(current)->accelertaion += fixedDeltaTime / 10;

	mParticles.at(current)->energy -= 0.01;

	if (mParticles.at(current)->energy <= 0.0f)
		ParticleReset(current);
}

void ParticleManager::BuildHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 11; 
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3ddevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3ddevice.Get(), 1, true);
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	md3ddevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
	mCbvHeap->SetName(L"Constant Buffer Heap");
}

void ParticleManager::BuildPSO()
{
	HRESULT hr = S_OK;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\cpuParVS.hlsl", nullptr, "VS", "vs_5_0");
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
	psoDesc.pRootSignature = rootSig.Get();
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
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality =  0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(md3ddevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

void ParticleManager::BuildRootSignature()
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

	ThrowIfFailed(md3ddevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSig)));
}