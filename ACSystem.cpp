#include "ACSystem.h"


ACSystem::ACSystem(ComPtr<ID3D12Device> &device, ComPtr<ID3D12CommandQueue>& queue, ID3D12GraphicsCommandList* commandList)
{
	mDevice = device;

	//Confirm what this is for
	queue->GetTimestampFrequency(&frequency);

	//Need to ensure correct value is being set here
	frameIndex = 0;
	previousIndex = frameIndex;

	list = commandList;

	m_computeFenceValue = 1;
	

	CreateComputeResources();
	CreateGraphicsResources();
	CreateTimingResources();	
}


ACSystem::~ACSystem()
{
}

void ACSystem::CreateComputeResources()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&computeQueue)));
	computeQueue->SetName(L"Compute Queue");

	for (int i = 0; i < frameCount; ++i)
	{
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&computeAllocator[i])));
		ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, computeAllocator[i].Get(), nullptr, IID_PPV_ARGS(&acComputeList[i])));
		ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&computeFence[i])));

		acComputeList[i]->Close();
	}
}

void ACSystem::CreateGraphicsResources()
{
	for (int i = 0; i < frameCount; ++i) {

		//Graphics
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&graphicsAllocator[i])));
		ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, graphicsAllocator[i].Get(), nullptr, IID_PPV_ARGS(&acGraphicsList[i])));
		ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&graphicsFence[i])));

		acGraphicsList[i]->Close();
		//Copy
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&copyAllocator[i])));
		ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, copyAllocator[i].Get(), nullptr, IID_PPV_ARGS(&acCopyList[i])));
		ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&copyFence[i])));

		acCopyList[i]->Close();
	}
}

void ACSystem::CreateTimingResources()
{
	D3D12_RESOURCE_DESC cpuTimingBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		sizeof(UINT64));

	for (int i = 0; i < frameCount; ++i) {
		mDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&cpuTimingBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&timeQueryReadBuffer[i]));
	}

	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
	queryHeapDesc.Count = frameCount; // Query at end of frame
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

	mDevice->CreateQueryHeap(&queryHeapDesc,
		IID_PPV_ARGS(&queryHeap));

}