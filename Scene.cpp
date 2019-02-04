#include "Scene.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::BuildScene(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
{

	//GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);


	//gridVertexOffset = 0;
	//gridIndexOffset = 0;

	//gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	//gridSubmesh.StartIndexLocation = gridIndexOffset;
	//gridSubmesh.BaseVertexLocation = gridVertexOffset;


	//GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);

	//sphereVertexOffset = (UINT)grid.Vertices.size();
	//sphereIndexOffset = (UINT)grid.Indices32.size();

	//sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	//sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	//sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	//auto totalVertexCount = grid.Vertices.size() + sphere.Vertices.size();

	//std::vector<Vertex> vertices(totalVertexCount);

	//UINT k = 0;

	//for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	//{
	//	vertices[k].Pos = grid.Vertices[i].Position;
	//	vertices[k].Color = XMFLOAT4(1, 1, 0, 1);
	//}

	//for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	//{
	//	vertices[k].Pos = sphere.Vertices[i].Position;
	//	vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
	//}

	//std::vector<std::uint16_t> indices;

	//indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	//indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

	//const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	//const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	////auto geo = std::make_unique<MeshGeometry>();
	//meshGeo = std::make_unique<MeshGeometry>();
	//meshGeo->Name = "shapeGeo";

	//ThrowIfFailed(D3DCreateBlob(vbByteSize, &meshGeo->VertexBufferCPU));
	//CopyMemory(meshGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	//ThrowIfFailed(D3DCreateBlob(ibByteSize, &meshGeo->IndexBufferCPU));
	//CopyMemory(meshGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	//meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
	//	commandList.Get(), vertices.data(), vbByteSize, meshGeo->VertexBufferUploader);

	//meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
	//	commandList.Get(), indices.data(), ibByteSize, meshGeo->IndexBufferUploader);

	//meshGeo->VertexByteStride = sizeof(Vertex);
	//meshGeo->VertexBufferByteSize = vbByteSize;
	//meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	//meshGeo->IndexBufferByteSize = ibByteSize;

	//meshGeo->DrawArgs["grid"] = gridSubmesh;
	//meshGeo->DrawArgs["sphere"] = sphereSubmesh;
}


