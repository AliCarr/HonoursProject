#pragma once

#include "stdafx.h"
#include <time.h>

class ACSystem
{
public:
	ACSystem(ComPtr<ID3D12Device>&, ComPtr<ID3D12CommandQueue>&, ID3D12GraphicsCommandList*);
	~ACSystem();

	static const int frameCount = 4;

	bool IsActive() { return active; };

	ComPtr<ID3D12CommandQueue> computeQueue;

	ComPtr<ID3D12CommandAllocator> uploadAllocator, graphicsAllocator[frameCount], copyAllocator[frameCount], computeAllocator[frameCount];
	ComPtr<ID3D12GraphicsCommandList> acComputeList[frameCount];

	ComPtr<ID3D12Fence> graphicsFence[frameCount], copyFence[frameCount], computeFence[frameCount];

	UINT frameIndex;
	UINT previousIndex;
	UINT64 copyFenceValues[frameCount];
	UINT64 computeFenceValues[frameCount];
	UINT64 renderFenceValues[frameCount];


	ComPtr<ID3D12GraphicsCommandList> acCopyList[frameCount];



	UINT64 m_computeFenceValue;
	UINT64 m_graphicsFenceValue;
	UINT64 m_graphicsCopyFenceValue;


private:

	time_t mTime;

	bool active = true;
	void CreateComputeResources();
	void CreateGraphicsResources();
	void CreateTimingResources();
	int currentNumberOfParticles = 1936;
	static const int numberOfParticles = 1936;


	
	UINT indexOffset;
	UINT vertexOffset;
	UINT indexCount;

	Vertex vert[numberOfParticles];
	std::vector<ParticleInfromation*> mParticles;
	std::vector<ComputeData> particleInputeData;

	ID3D12GraphicsCommandList* list;

	ComPtr<ID3D12GraphicsCommandList> acGraphicsList[frameCount];

	ComPtr<ID3D12Resource> timeQueryReadBuffer[frameCount];
	ComPtr<ID3D12QueryHeap> queryHeap;







	ComPtr<ID3D12Device> mDevice;

	UINT64 frequency;

};

