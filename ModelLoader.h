#pragma once
#include "stdafx.h"
#include "tinyobjloader-master\tiny_obj_loader.h"

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();

	bool LoadModel(Microsoft::WRL::ComPtr<ID3D12Device>*, ID3D12GraphicsCommandList*);

private:

	const std::string MODEL_PATH = "Assets/Mount Wario.obj";
	const std::string TEXTURE_PATH = "Assets/chalet.jpg";

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	bool FillVectors();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
};

