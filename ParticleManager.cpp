#include "ParticleManager.h"
#include <cstdlib>

ParticleManager::ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	assert(GenerateParticleMesh(device, commandList));

	srand((unsigned)time(&mTime));

	for (int c = 0; c < numberOfParticles; c++)
	{
		auto par = new ParticleInfromation();

		par->geo = new MeshGeometry(*mGeo);
		par->position = StartingPosition();
		par->dynamicVB = std::make_unique<UploadBuffer<Vertex>>(device.Get(), indexCount, false);
		par->velocity = StartingVelocity();
		par->accelertaion = 0;
		par->energy = 5;
		mParticles.push_back(std::move(par));
	}
}


ParticleManager::~ParticleManager()
{
	for (int c = 0; c < numberOfParticles; c++)
	{
		delete mParticles.at(c);
		mParticles.at(c) = 0;
	}
}

void ParticleManager::Update(XMMATRIX& mat, float time, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, Microsoft::WRL::ComPtr<ID3D12Device> &device)
{
	for (int c = 0; c < numberOfParticles; c++)
	{
		auto currVB = mParticles.at(c)->dynamicVB.get();

		mParticles.at(c)->energy -= time;

		UpdatePosition(c, time, currVB);

		mParticles.at(c)->geo->VertexBufferGPU = currVB->Resource();
	}
}

void ParticleManager::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12DescriptorHeap> &heap, UINT size, Microsoft::WRL::ComPtr<ID3D12Device> &device)
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

}

XMFLOAT3 ParticleManager::StartingVelocity()
{
	return XMFLOAT3{ ((float)(rand() % 300) - 150) / 100,
		((float)(rand() % 300) - 150) / 100,
		((float)(rand() % 300) - 150) / 100 };
}

XMFLOAT3 ParticleManager::StartingPosition()
{
	return XMFLOAT3{ ((float)(rand() % 200)) / 500,
		((float)(rand() % 200)) / 400,
		((float)(rand() % 200)) / 500 };
}

void ParticleManager::ParticleReset(int current)
{
	mParticles.at(current)->energy = 5;
	mParticles.at(current)->position = StartingPosition();
	mParticles.at(current)->accelertaion = 0;
	mParticles.at(current)->velocity = StartingVelocity();
}

bool ParticleManager::GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
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
		commandList.Get(),
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
		mParticles.at(current)->accelertaion += time / 30;
	for (int i = 0; i < vertexOffset; i++)
	{
		//Offset the cooridnates for each vertex
		mParticles.at(current)->position.x += vert[i].Pos.x + (mParticles.at(current)->velocity.x*(time / speed));
		mParticles.at(current)->position.y += vert[i].Pos.y + (mParticles.at(current)->velocity.y*(time / speed)) - mParticles.at(current)->accelertaion;
		mParticles.at(current)->position.z += vert[i].Pos.z + (mParticles.at(current)->velocity.z*(time / speed));

		Vertex v;
		v.Pos = mParticles.at(current)->position;
		v.Color = XMFLOAT4(v.Pos.x, v.Pos.y, v.Pos.z, 0.0f); //Change colour based on the position
		v.texCoord = { 0, 0 };
		buffer->CopyData(i, v);
	}

	if (mParticles.at(current)->position.y <= -3)
	{
		mParticles.at(current)->accelertaion = -mParticles.at(current)->accelertaion / 2.2;
		mParticles.at(current)->velocity.x /= 1.1;
		mParticles.at(current)->velocity.z /= 1.1;
	}

	if (mParticles.at(current)->energy <= 0)
		ParticleReset(current);
}