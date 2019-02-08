#include "ParticleManager.h"



ParticleManager::ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	/*for(int c = 0; c < numberOfParticles; c++)
		mParticle[c] = new Particle(mesh,device, commandList);*/

	for (int c = 0; c < numberOfParticles; c++)
	{
		auto par = new Particle(mesh, device, commandList);
		//(&par->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));

		//par->Geo = mesh.get();
		par->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		par->IndexCount = par->Geo->DrawArgs["particle"].IndexCount;
		par->ObjCBIndex = c;
		par->StartIndexLocation = par->Geo->DrawArgs["particle"].StartIndexLocation;
		par->BaseVertexLocation = par->Geo->DrawArgs["particle"].BaseVertexLocation;
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
		mParticles.at(c)->update(time);
	}

	//mat = mParticles.at(0)->World;
}

void ParticleManager::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12DescriptorHeap> &heap, UINT size)
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

void ParticleManager::UpdateCBuffers(std::unique_ptr<UploadBuffer<ObjectConstants>> &mObjectCB)
{
	

	for(int c = 0; c < numberOfParticles; c++)
	{
		ObjectConstants objCons;
		  XMStoreFloat4x4(&objCons.WorldViewProj, mParticles.at(c)->World);

		mObjectCB->CopyData(mParticles.at(c)->ObjCBIndex, objCons);
	}

	
}


