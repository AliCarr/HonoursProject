#pragma once

#include "stdafx.h"
#include <time.h>

class ACParticleSystem
{
	public:
		ACParticleSystem(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList* commandList, ComPtr<ID3D12CommandQueue> &graphicsQueue);
		~ACParticleSystem();

		void Render(ComPtr<ID3D12DescriptorHeap> &heap);

		void Execute(ComPtr<ID3D12CommandQueue> graphicsQueue,
			ComPtr<ID3D12Resource>& drawBuffer, ComPtr<IDXGISwapChain> mSwapChain);

		void BuildDescriptors(UINT descriptorSize, ComPtr<ID3D12DescriptorHeap>&, ID3D12GraphicsCommandList*);

		ComPtr<ID3D12RootSignature> GetComputeRootSignature() { return mComputeRootSignature; };
		UINT SRV = 0U;

		ID3D12Resource *pUavResource;



	private:
		static const int FrameCount = 4;

		time_t mTime;
		UINT indexOffset;
		UINT vertexOffset;
		UINT indexCount;

		static const int numberOfParticles = 2760;

		int currentNumberOfParticles = numberOfParticles;

		ComPtr<ID3D12RootSignature> mComputeRootSignature = nullptr;
		void CreateBuffers(Microsoft::WRL::ComPtr<ID3D12Device> &, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);


		void CreateRootSignatures();
		bool GenerateParticleMesh(Microsoft::WRL::ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList);
		void UpdatePosition(int, float, UploadBuffer<Vertex>*);
		void ParticleReset(int);
		void BuildResources();
		void BuildPSO(ComPtr<ID3DBlob> mcsByteCode, ComPtr<ID3D12PipelineState > &mPSO);
		void BuildACObjects();

		unsigned long long totalVertexCount;

		MeshGeometry *mGeo = nullptr;
		std::vector<ParticleInfromation*> mParticles;
		GeometryGenerator generator;
		SubmeshGeometry boxSubmesh;
		Vertex vert[numberOfParticles];
		const float width = 0.008f;
		const float depth = 0.0004f;
		const UINT32 rows = 2;
		const UINT32 columns = 2;
		std::vector<ComputeData> particleInputeData;

		ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;

		XMFLOAT3 StartingVelocity();
		XMFLOAT3 StartingPosition();
		UINT m_srvUavDescriptorSize;


		ComPtr<ID3D12Resource> inputParticleBuffer;
		ComPtr<ID3D12Resource> inputParticleBuffer2;
		ComPtr<ID3D12Resource> outputParticleBuffer;
		ComPtr<ID3D12Resource> uploadParticleBuffer;
		ComPtr<ID3D12Resource> uploadParticleBuffer2;

		ComPtr<ID3D12RootSignature> m_rootSignature;

		Microsoft::WRL::ComPtr<ID3D12Device> md3ddevice;
		ID3D12GraphicsCommandList* list;

		std::unique_ptr<UploadBuffer<Vertex>> uploader = nullptr;
		bool whichHandle = false;

		UINT UAV = 1U;

		// Compute objects.
		ComPtr<ID3D12CommandAllocator> m_computeAllocators[FrameCount];
		ComPtr<ID3D12CommandQueue> m_computeCommandQueue;
		ComPtr<ID3D12GraphicsCommandList> m_computeCommandLists[FrameCount];


		// Synchronization objects.
		ComPtr<ID3D12Fence>	m_computeFences[FrameCount];
		ComPtr<ID3D12Fence> m_graphicsFences[FrameCount];
		ComPtr<ID3D12Fence> m_graphicsCopyFences[FrameCount];

		UINT64 m_computeFenceValue;
		UINT64 m_graphicsFenceValue;
		UINT64 m_graphicsCopyFenceValue;
		UINT64 m_computeFenceValues[FrameCount];
		UINT64 m_graphicsFenceValues[FrameCount];
		UINT64 m_graphicsCopyFenceValues[FrameCount];

		HANDLE m_computeFenceEvents[FrameCount];
		HANDLE m_graphicsFenceEvents[FrameCount];
		HANDLE m_graphicsCopyFenceEvents[FrameCount];

		UINT64 m_frameFenceValue;
		UINT64 m_frameFenceValues[FrameCount];
		ComPtr<ID3D12Fence> m_frameFences[FrameCount];
		HANDLE m_frameFenceEvents[FrameCount];

		ComPtr<ID3D12CommandAllocator> m_graphicsAllocators[FrameCount];
		ComPtr<ID3D12GraphicsCommandList> m_graphicsCommandLists[FrameCount];
		ComPtr<ID3D12CommandAllocator> m_graphicsCopyAllocators[FrameCount];
		ComPtr<ID3D12GraphicsCommandList> m_graphicsCopyCommandLists[FrameCount];

		// Timing queries
		ComPtr<ID3D12QueryHeap> m_timeQueryHeap;
		ComPtr<ID3D12Resource> m_timeQueryReadbackBuffer[FrameCount];
		UINT64 m_queryResults[FrameCount];
		int m_queryReadbackIndex;
		UINT64 m_frequency;

		UINT64 frameIndex, lastFrameIndex;

		void RecordComputeTasks();
		void RecordCopyTasks(ComPtr<ID3D12Resource>&);
		void RecordRenderTasks(ComPtr<IDXGISwapChain>);
		ComPtr<ID3D12PipelineState> pso, computePso;
		ComPtr<ID3D12DescriptorHeap> computeHeap, cbvHeap;
		ComPtr<ID3D12RootSignature> renderSig, computeSig;

		void BuildHeaps();
		void BuildPSOs();
		void BuildRootSignatures();

		ComPtr<ID3DBlob> mcsByteCode = nullptr;
		ComPtr<ID3DBlob> mvsByteCode = nullptr;
		ComPtr<ID3DBlob> mpsByteCode = nullptr;
		std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
		ID3D12DescriptorHeap* descriptorHeaps[2];
		void BuildConstantBuffers();

		std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
		ComPtr<ID3D12Resource> m_particleBufferForDraw;

		//void WaitForFence(ID3D12Fence* fence, UINT64 fenceValue, HANDLE fenceEvent);

		ComPtr<ID3D12GraphicsCommandList> m_uploadCommandList;
		ComPtr<ID3D12CommandAllocator> m_uploadCommandAllocator;
		ComPtr<ID3D12Fence> m_uploadFence;
		HANDLE m_uploadEvent;
		UINT64 m_uploadFenceValue;

		ComPtr<ID3D12CommandQueue> mGraphicsQueue;

};