#include "Particle.h"

Particle::Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, float offset)
{
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(width, height, 60, 40);

	gridVertexOffset = 0;
	gridIndexOffset = 0;

	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	auto totalVertexCount = grid.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;

	float random = ((rand() % 20)/10) - 1;

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Pos.x += offset;
		vertices[k].Color = XMFLOAT4(1, 1, 0, 1);
		vertices[k].Velocity = 1.f;
		vertices[k].active = 1.0f;
	}

	std::vector<std::uint16_t> indices;

	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	//auto geo = std::make_unique<MeshGeometry>();
	meshGeo = std::make_unique<MeshGeometry>();
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

	meshGeo->DrawArgs["grid"] = gridSubmesh;



	position.x = rand() % 1 - 0.5f;;
	position.y = 1;
	position.z = 0;

	velocity = 0.001f;
}


Particle::~Particle()
{
}

float Particle::update(XMMATRIX &world, float deltaTime)
{
	return position.y -= velocity * deltaTime;

	
}
