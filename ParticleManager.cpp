#include "ParticleManager.h"



ParticleManager::ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	/*for(int c = 0; c < numberOfParticles; c++)
		mParticle[c] = new Particle(mesh,device, commandList);*/

	meshing = geoGen.CreateGrid(1, 1, 2, 2);
	
	gridSubmesh.IndexCount = (UINT)meshing.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	auto totalVertexCount = meshing.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;

	float random = ((rand() % 20) / 10) - 1;

	for (size_t i = 0; i < meshing.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = meshing.Vertices[i].Position;
		vertices[k].texCoord = XMFLOAT2(1, 1);
		vertices[k].Color = XMFLOAT4(1, 1, 0, 1);
	}

	std::vector<std::uint16_t> indices;

	indices.insert(indices.end(), std::begin(meshing.GetIndices16()), std::end(meshing.GetIndices16()));


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	meshGeo = std::make_unique<MeshGeometry>();
	//auto geo = std::make_unique<MeshGeometry>();
	//geo = new MeshGeometry();
	meshGeo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &meshGeo->VertexBufferCPU));
	CopyMemory(meshGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &meshGeo->IndexBufferCPU));
	CopyMemory(meshGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), vertices.data(), vbByteSize, meshGeo->VertexBufferUploader);

	meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), indices.data(), ibByteSize, meshGeo->IndexBufferUploader);

	meshGeo->VertexByteStride = sizeof(Vertex);
	meshGeo->VertexBufferByteSize = vbByteSize;
	meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	meshGeo->IndexBufferByteSize = ibByteSize;

	meshGeo->DrawArgs["particle"] = gridSubmesh;




	for (int c = 0; c < numberOfParticles; c++)
	{
		auto par = std::make_unique<ParticleItems>();
		//(&par->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));

		par->Geo = meshGeo.get();
		par->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		//par->IndexCount = par->Geo->DrawArgs["particle"].IndexCount;
		par->ObjCBIndex = c;
		par->StartIndexLocation = meshGeo->DrawArgs["particle"].StartIndexLocation;
		par->BaseVertexLocation = meshGeo->DrawArgs["particle"].BaseVertexLocation;
		mParticles.push_back(std::move(par));
	}

	float test = 0;
	test += 2;
}


ParticleManager::~ParticleManager()
{
}

void ParticleManager::Update(XMMATRIX& mat, float time)
{
	//XMMATRIX translate = XMMatrixTranslation(mParticle[0]->update(mat, time).x, mParticle[0]->update(mat, time).y, mParticle[0]->update(mat, time).z);
	
	//mat = XMMatrixMultiply(mat, translate);

	//1.1 check if active
	//1.2 if it is, caclulate new position based on particles update function
	//1.3 else continue normally 
	//NOTE: This will also be a loop as it goes through all particles
	//Future consideration might have a list of active and a list of inactive so that you don't deal with one that isn't needed

	for (int c = 0; c < numberOfParticles; c++)
	{
		mParticles.at(c)->position.y -= mParticles.at(c)->velocity * time;
	}

	//mat = mParticles.at(0)->World;
}

void ParticleManager::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12DescriptorHeap> &heap, UINT size, Microsoft::WRL::ComPtr<ID3D12Device> &device, std::unique_ptr<MeshGeometry>& mesh)
{
	//0.0 Pass in command list 
	//1.0 Start loop
	//1.05 check if particle is active or not
	//1.1 For the size of my particle vector
	//1.2 take current number as point in vector we want
	//2.0 Set vertex buffers to VertexBufferView of current particle
	//2.1 do the same for index and primitive topology if needed
	//2.2 set graphics root D table
	//2.3 Draw indexed instanced

	for (int c = 0; c < numberOfParticles; c++)
	{

		//Update the psitions.
		auto totalVertexCount = meshing.Vertices.size();

		std::vector<Vertex> vertices(totalVertexCount);

		UINT k = 0;

		for (size_t i = 0; i < meshing.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos.x = meshing.Vertices[i].Position.x + mParticles.at(c)->position.x;
			vertices[k].Pos.y = meshing.Vertices[i].Position.y + mParticles.at(c)->position.y;
			vertices[k].Pos.z = meshing.Vertices[i].Position.z + mParticles.at(c)->position.z;

		}
		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);


		ThrowIfFailed(D3DCreateBlob(vbByteSize, &mParticles.at(c)->Geo->VertexBufferCPU));
		CopyMemory(mParticles.at(c)->Geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		mParticles.at(c)->Geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
			commandList.Get(), vertices.data(), vbByteSize, mParticles.at(c)->Geo->VertexBufferUploader);








		////RENDERING STARTS HERE
		commandList->IASetVertexBuffers(0, 1, &mParticles.at(c)->Geo->VertexBufferView());

		commandList->IASetIndexBuffer(&mParticles.at(c)->Geo->IndexBufferView());
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	/*	UINT cbvIndex =  (UINT)mParticles.size() * mParticles.at(c)->ObjCBIndex;
		auto cbvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, size);*/

		commandList->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());

		commandList->DrawIndexedInstanced(mParticles.at(c)->Geo->DrawArgs["particle"].IndexCount,
										  1, 
			mParticles.at(c)->Geo->DrawArgs["particle"].StartIndexLocation,
			mParticles.at(c)->Geo->DrawArgs["particle"].BaseVertexLocation,
										  0);

	}


}



