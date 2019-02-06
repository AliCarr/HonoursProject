#include "ParticleManager.h"



ParticleManager::ParticleManager(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry>& mesh)
{
	for(int c = 0; c < numberOfParticles; c++)
		mParticle[c] = new Particle(mesh,device, commandList);
}


ParticleManager::~ParticleManager()
{
}

void ParticleManager::Update(XMMATRIX& mat, float time)
{
	XMMATRIX translate = XMMatrixTranslation(mParticle[0]->update(mat, time).x, mParticle[0]->update(mat, time).y, mParticle[0]->update(mat, time).z);
	
	mat = XMMatrixMultiply(mat, translate);
}

void ParticleManager::Render()
{
	//1. check if particle is active
	//2. if not, either stop it being rendered, or return it to start position

}


