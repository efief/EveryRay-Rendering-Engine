#include "stdafx.h"

#include "TerrainMaterial.h"
#include "GameException.h"
#include "Mesh.h"
#include "ColorHelper.h"

namespace Library
{
	RTTI_DEFINITIONS(TerrainMaterial)

	TerrainMaterial::TerrainMaterial() : Material("main"),
		MATERIAL_VARIABLE_INITIALIZATION(World),
		MATERIAL_VARIABLE_INITIALIZATION(View),
		MATERIAL_VARIABLE_INITIALIZATION(Projection),
		MATERIAL_VARIABLE_INITIALIZATION(albedoTexture),
		MATERIAL_VARIABLE_INITIALIZATION(SunDirection),
		MATERIAL_VARIABLE_INITIALIZATION(SunColor),
		MATERIAL_VARIABLE_INITIALIZATION(AmbientColor),
		MATERIAL_VARIABLE_INITIALIZATION(ShadowCascadeDistances),
		MATERIAL_VARIABLE_INITIALIZATION(ShadowTexelSize)
	{
	}

	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, World)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, View)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, Projection)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, albedoTexture)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, SunDirection)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, SunColor)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, AmbientColor)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, ShadowCascadeDistances)
	MATERIAL_VARIABLE_DEFINITION(TerrainMaterial, ShadowTexelSize)

	void TerrainMaterial::Initialize(Effect* effect)
	{
		Material::Initialize(effect);

		MATERIAL_VARIABLE_RETRIEVE(World)
		MATERIAL_VARIABLE_RETRIEVE(View)
		MATERIAL_VARIABLE_RETRIEVE(Projection)
		MATERIAL_VARIABLE_RETRIEVE(albedoTexture)
			MATERIAL_VARIABLE_RETRIEVE(SunDirection)
			MATERIAL_VARIABLE_RETRIEVE(SunColor)
			MATERIAL_VARIABLE_RETRIEVE(AmbientColor)
			MATERIAL_VARIABLE_RETRIEVE(ShadowCascadeDistances)
			MATERIAL_VARIABLE_RETRIEVE(ShadowTexelSize)

		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		CreateInputLayout("main", "p0", inputElementDescriptions, ARRAYSIZE(inputElementDescriptions));
	}

	void TerrainMaterial::CreateVertexBuffer(ID3D11Device* device, const Mesh& mesh, ID3D11Buffer** vertexBuffer) const
	{
		const std::vector<XMFLOAT3>& sourceVertices = mesh.Vertices();
		std::vector<XMFLOAT3>* textureCoordinates = mesh.TextureCoordinates().at(0);
		const std::vector<XMFLOAT3>& normals = mesh.Normals();

		std::vector<VertexPositionTextureNormal> vertices;
		vertices.reserve(sourceVertices.size());
		for (UINT i = 0; i < sourceVertices.size(); i++)
		{
			XMFLOAT3 position = sourceVertices.at(i);
			XMFLOAT3 uv = textureCoordinates->at(i);
			XMFLOAT3 normal = normals.at(i);
			vertices.push_back(VertexPositionTextureNormal(XMFLOAT4(position.x, position.y, position.z, 1.0f), XMFLOAT2(uv.x, uv.y), normal));
		}

		CreateVertexBuffer(device, &vertices[0], vertices.size(), vertexBuffer);
	}

	void TerrainMaterial::CreateVertexBuffer(ID3D11Device* device, VertexPositionTextureNormal* vertices, UINT vertexCount, ID3D11Buffer** vertexBuffer) const
	{
		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
		vertexBufferDesc.ByteWidth = VertexSize() * vertexCount;
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData;
		ZeroMemory(&vertexSubResourceData, sizeof(vertexSubResourceData));
		vertexSubResourceData.pSysMem = vertices;
		if (FAILED(device->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, vertexBuffer)))
		{
			throw GameException("ID3D11Device::CreateBuffer() failed.");
		}
	}

	UINT TerrainMaterial::VertexSize() const
	{
		return sizeof(VertexPositionTextureNormal);
	}
}