#include "Particle.h"

Particle::Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
{
	mesh = geoGen.CreateGrid(width, height, 60, 40);

	gridVertexOffset = 0;
	gridIndexOffset = 0;
	position.y = 1;
	position = {((float)(rand() % 50) / 40.0f), 1.0f, ((float)(rand() % 50) / 40.0f) };
	CreateParticle(meshGeo, device, commandList);

	isActive = true;
}


Particle::~Particle()
{
}

XMFLOAT3 Particle::update(XMMATRIX &world, float deltaTime)
{
	position.y -= velocity * deltaTime;

	if (position.y <= -4)
	{
		//For now just have it return to start. Better handling can be added later
		position.y = 1;
		isActive = false;
	}
	return position;
}

bool Particle::CreateParticle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
{

	gridSubmesh.IndexCount = (UINT)mesh.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	auto totalVertexCount = mesh.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;

	float random = ((rand() % 20) / 10) - 1;

	for (size_t i = 0; i < mesh.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = mesh.Vertices[i].Position;
		vertices[k].texCoord = XMFLOAT2(1, 1);
		vertices[k].Color = XMFLOAT4(1, 1, 0, 1);
	}

	std::vector<std::uint16_t> indices;

	indices.insert(indices.end(), std::begin(mesh.GetIndices16()), std::end(mesh.GetIndices16()));


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

	return true;
}