#pragma once
#include "stdafx.h"
#include "tinyobjloader-master\tiny_obj_loader.h"

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();

	//Functions
	bool LoadModel(Microsoft::WRL::ComPtr<ID3D12Device>*, ID3D12GraphicsCommandList*);

private:
	//Functions
	bool FillVectors();

	//Strings
	const std::string MODEL_PATH = "Assets/Mount Wario.obj";
	const std::string TEXTURE_PATH = "Assets/chalet.jpg";
	std::string warn, err;

	//Model Objects
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
};

