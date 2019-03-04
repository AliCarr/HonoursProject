#include "ParticleManager.h"



ParticleManager::ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	mGeo = std::make_unique<MeshGeometry>();
	mGeo->Name = "shapeGeo";
	GeometryGenerator generator;
	//Create geometry generator for the particle (will use the create grid function)
	GeometryGenerator::MeshData particleMeshData = generator.CreateGrid(1.0f, 1.0f, 2.0f, 2.0f);
	//Create the offset value for vertex and index
	vertexOffset = (UINT)particleMeshData.Vertices.size();
	indexOffset = (UINT)particleMeshData.Indices32.size();
	indexCount = (UINT)particleMeshData.Indices32.size();

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)particleMeshData.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;
	//Get the total vertex count (this is the number of particles multiplied by the vertex size
	totalVertexCount = vertexOffset;
	//Use this size to create vertex vector 
	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;

	UINT k = 0;
	//for (int c = 0; c < numberOfParticles; c++)
	//{

	for (size_t i = 0; i < vertexOffset; i++, k++)
	{
		vertices[k].Pos = particleMeshData.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
	}
	indices.insert(indices.end(),
		std::begin(particleMeshData.GetIndices16()),
		std::end(particleMeshData.GetIndices16()));
	//}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);


	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeo->VertexBufferCPU));
	CopyMemory(mGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeo->IndexBufferCPU));
	CopyMemory(mGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), vertices.data(), vbByteSize, mGeo->VertexBufferUploader);

	mGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), indices.data(), ibByteSize, mGeo->IndexBufferUploader);

	mGeo->VertexByteStride = sizeof(Vertex);
	mGeo->VertexBufferByteSize = vbByteSize;
	mGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mGeo->IndexBufferByteSize = ibByteSize;

	mGeo->DrawArgs["particle"] = boxSubmesh;


	for (int c = 0; c < numberOfParticles; c++)
	{
		auto par = new ParticleInfromation();

		//par = new ParticleInfromation();
		par->geo = new MeshGeometry();
		par->geo = mGeo.get();
		par->IndexCount = mGeo->DrawArgs["particle"].IndexCount;
		par->StartIndexLocation = mGeo->DrawArgs["particle"].StartIndexLocation;
		par->BaseVertexLocation = mGeo->DrawArgs["particle"].BaseVertexLocation;
		par->vbByteSize = vbByteSize;
		par->position = { 0, 0, 0 };
		par->dynamicVB = std::make_unique<UploadBuffer<Vertex>>(device.Get(), 6, false);
		
		mParticles.push_back(std::move(par));
	}
}


ParticleManager::~ParticleManager()
{
}

void ParticleManager::Update(XMMATRIX& mat, float time, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, Microsoft::WRL::ComPtr<ID3D12Device> &device)
{


	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;


	UINT k = 0;

	for (int c = 0; c < numberOfParticles; c++)
	{
		auto currVB = mParticles.at(c)->dynamicVB.get();
	/*	mParticles.at(c)->geo->VertexBufferGPU = nullptr;*/
		std::vector<Vertex> newV(totalVertexCount);
		for (int i = 0; i < vertexOffset; i++)
		{
			mParticles.at(c)->position.x = 0.0;
			mParticles.at(c)->position.y = 0.0;
			mParticles.at(c)->position.z = 0.0;
			Vertex v;
				v.Pos = {1, 0, 0};
				v.Color = XMFLOAT4(DirectX::Colors::Crimson);
				v.texCoord = { 1, 1 };
			currVB->CopyData(i, v);
		}

		//	ThrowIfFailed(D3DCreateBlob(mParticle[c]->vbByteSize, &mParticle[c]->geo->VertexBufferCPU));
		//	CopyMemory(mParticle[c]->geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), mParticle[c]->vbByteSize);




		mParticles.at(c)->geo->VertexBufferGPU = currVB->Resource();
	
		//currVB = NULL;
		//mGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		//	commandList.Get(), vertices.data(), vbByteSize, mGeo->VertexBufferUploader);
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

void ParticleManager::UpdateCBuffers(std::unique_ptr<UploadBuffer<ObjectConstants>> &mObjectCB)
{
	//for (int c = 0; c < numberOfParticles; c++)
	//{
	//	ObjectConstants objCons;
	//	XMStoreFloat4x4(&objCons.WorldViewProj, mParticles.at(c)->World);

	//	mObjectCB->CopyData(mParticles.at(c)->ObjCBIndex, objCons);
	//}
}