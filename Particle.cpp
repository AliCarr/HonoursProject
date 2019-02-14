#include "Particle.h"

Particle::Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
{
	mesh = geoGen.CreateGrid(width, height, 4, 4);

	gridVertexOffset = 0;
	gridIndexOffset = 0;
	position.y = 1;
	position = { ((float)(rand() % 3)), 1.0f, ((float)(rand() % 50) / 40.0f) };
	CreateParticle(device, commandList);
	World = XMMatrixTranslation(position.x, position.y, position.z);
	isActive = true;
}


Particle::~Particle()
{
}

XMFLOAT3 Particle::update(float deltaTime)
{
	position.y -= velocity * deltaTime;

	if (position.y <= -4)
	{
		//For now just have it return to start. Better handling can be added later
		position.y = 1;
		isActive = false;
	}

	World = XMMatrixTranslation(position.x, position.y, position.z);


	return position;
}

bool Particle::CreateParticle(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
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
	Geo = new MeshGeometry();
	Geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &Geo->VertexBufferCPU));
	CopyMemory(Geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &Geo->IndexBufferCPU));
	CopyMemory(Geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	Geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), vertices.data(), vbByteSize, Geo->VertexBufferUploader);

	Geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), indices.data(), ibByteSize, Geo->IndexBufferUploader);

	Geo->VertexByteStride = sizeof(Vertex);
	Geo->VertexBufferByteSize = vbByteSize;
	Geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	Geo->IndexBufferByteSize = ibByteSize;

	Geo->DrawArgs["particle"] = gridSubmesh;

	return true;
}

void Particle::updateGeo(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
{
	auto totalVertexCount = mesh.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;

	for (size_t i = 0; i < mesh.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos.x = mesh.Vertices[i].Position.x + position.x  ;
		vertices[k].Pos.y = mesh.Vertices[i].Position.y + position.y  ;
		vertices[k].Pos.z = mesh.Vertices[i].Position.z + position.z  ;
		//vertices[k].Pos = position;
		vertices[k].texCoord = XMFLOAT2(1, 1);
		vertices[k].Color = XMFLOAT4(1, 1, 0, 1);
	}
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	Geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), vertices.data(), vbByteSize, Geo->VertexBufferUploader);
}