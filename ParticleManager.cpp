#include "ParticleManager.h"
#include <cstdlib>

ParticleManager::ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	GenerateParticleMesh(device, commandList);

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

		auto data = new ComputeData();

		data->initialPosition = par->position;
		data->position = par->position;
		data->velocity = par->velocity;

		particleInputeData.push_back(std::move(data));
 	}

	UINT64	byteSize = particleInputeData.size() * sizeof(ComputeData);

	mInputBuffer = d3dUtil::CreateDefaultBuffer(device.Get(), commandList, particleInputeData.data(), byteSize, mInputUploadBuffer);

	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), 
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
		nullptr, 
		IID_PPV_ARGS(&mOutputBuffer)));

	//assert(mOutputBuffer == NULL);

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

void ParticleManager::Update(XMMATRIX& mat, float time, ID3D12GraphicsCommandList* commandList, Microsoft::WRL::ComPtr<ID3D12Device> &device)
{
	for (int c = 0; c < numberOfParticles; c++)
	{
		auto currVB = mParticles.at(c)->dynamicVB.get();
			mParticles.at(c)->energy -= time;
			UpdatePosition(c, time, currVB);
			mParticles.at(c)->geo->VertexBufferGPU = currVB->Resource();
	}

	//assert(mOutputBuffer == NULL);
	//assert(mOutputBuffer->GetGPUVirtualAddress() == NULL);

	commandList->SetComputeRootShaderResourceView(0, mInputBuffer->GetGPUVirtualAddress());
	commandList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());

	commandList->Dispatch(32, 1, 1);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));

	//commandList->CopyResource(mInputUploadBuffer.Get(), mOutputBuffer.Get());

	mOutputBuffer->Map(0, nullptr, reinterpret_cast<void**>(&particleInputeData));
}

void ParticleManager::Render(ID3D12GraphicsCommandList *commandList, ComPtr<ID3D12DescriptorHeap> &heap, UINT size, Microsoft::WRL::ComPtr<ID3D12Device> &device, ComPtr<ID3D12PipelineState> pso)
{
	for (int c = 0; c < numberOfParticles; c++)
	{
		commandList->IASetVertexBuffers(0, 1, &mParticles.at(c)->geo->VertexBufferView());

		commandList->IASetIndexBuffer(&mParticles.at(c)->geo->IndexBufferView());
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

		commandList->DrawIndexedInstanced(mParticles.at(c)->geo->DrawArgs["particle"].IndexCount,
										  1,
										  mParticles.at(c)->geo->DrawArgs["particle"].StartIndexLocation,
										  mParticles.at(c)->geo->DrawArgs["particle"].BaseVertexLocation,
										  0);
	}

	//commandList->SetPipelineState(pso.Get());
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
	mParticles.at(current)->energy = ((float)(rand() % 300) + 100.0f) / 100.0f;
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

void ParticleManager::UpdatePosition(int current, float time, UploadBuffer<Vertex>* buffer)
{

	if(mParticles.at(current)->accelertaion <= maxAcceleration)
		mParticles.at(current)->accelertaion += time / 30.0f;

	for (UINT i = 0; i < vertexOffset; i++)
	{
		//Offset the cooridnates for each vertex
		mParticles.at(current)->position.x += vert[i].Pos.x + (mParticles.at(current)->velocity.x*(time / speed));
		mParticles.at(current)->position.y += vert[i].Pos.y + (mParticles.at(current)->velocity.y*(time / speed)) - mParticles.at(current)->accelertaion;
		mParticles.at(current)->position.z += vert[i].Pos.z + (mParticles.at(current)->velocity.z*(time / speed));

		Vertex v;
		v.Pos = mParticles.at(current)->position;
		v.Color = XMFLOAT4(DirectX::Colors::Blue); //Change colour based on the position
		v.texCoord = { 0.0f, 0.0f };
		buffer->CopyData(i, v);
		mParticles.at(current)->velocity.x += 0.001f;
		mParticles.at(current)->velocity.z += 0.001f;
	}

	if (mParticles.at(current)->position.y <= -3.0f)
	{
		mParticles.at(current)->accelertaion = -mParticles.at(current)->accelertaion / 2.2f;
		mParticles.at(current)->velocity.x /= 1.2f;
		mParticles.at(current)->velocity.z /= 1.2f;
		mParticles.at(current)->position.y = -2.8f;
	}

	if (mParticles.at(current)->energy <= 0.0f)
		ParticleReset(current);
}