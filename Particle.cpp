#include "Particle.h"

Particle::Particle(std::unique_ptr<MeshGeometry> &meshGeo, Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList)
{
	mesh = geoGen.CreateGrid(width, height, 2, 2);

	gridVertexOffset = 0;
	gridIndexOffset = 0;
	position.y = 1;
	position = {((float)(rand() % 50) / 46.0f), 1.0f + ((float) (rand()%370 / 359)), ((float)(rand() % 50) / 46.0f) };
	CreateParticle( device, commandList, meshGeo);
	World = XMMatrixTranslation(position.x, position.y, position.z);
	isActive = true;
}


Particle::~Particle()
{
}

XMFLOAT3 Particle::update( float deltaTime)
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

bool Particle::CreateParticle( Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry> &meshGeo)
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
	//meshGeo = new MeshGeometry();
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
	
	return true;
}

void Particle::updateGeo(Microsoft::WRL::ComPtr<ID3D12Device> &device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &commandList, std::unique_ptr<MeshGeometry> &meshGeo)
{
	auto totalVertexCount = mesh.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;

	for (size_t i = 0; i < mesh.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos.x = mesh.Vertices[i].Position.x + position.x  ;
		vertices[k].Pos.y = mesh.Vertices[i].Position.y + position.y  ;
		vertices[k].Pos.z = mesh.Vertices[i].Position.z + position.z  ;

	}
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);


	ThrowIfFailed(D3DCreateBlob(vbByteSize, &meshGeo->VertexBufferCPU));
	CopyMemory(meshGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
		commandList.Get(), vertices.data(), vbByteSize, meshGeo->VertexBufferUploader);
}