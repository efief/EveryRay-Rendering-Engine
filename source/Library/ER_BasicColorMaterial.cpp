#include "ER_BasicColorMaterial.h"
#include "ShaderCompiler.h"
#include "Utility.h"
#include "GameException.h"
#include "Game.h"
#include "RenderingObject.h"

namespace Library
{
	ER_BasicColorMaterial::ER_BasicColorMaterial(Game& game, unsigned int shaderFlags)
		: ER_Material(game, shaderFlags)
	{

		if (shaderFlags & HAS_VERTEX_SHADER)
		{
			D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			ER_Material::CreateVertexShader("content\\shaders\\BasicColor.hlsl", inputElementDescriptions, ARRAYSIZE(inputElementDescriptions));
		}
		
		if (shaderFlags & HAS_PIXEL_SHADER)
			ER_Material::CreatePixelShader("content\\shaders\\BasicColor.hlsl");

		mConstantBuffer.Initialize(ER_Material::GetGame()->Direct3DDevice());
	}

	ER_BasicColorMaterial::~ER_BasicColorMaterial()
	{
		mConstantBuffer.Release();
		ER_Material::~ER_Material();
	}

	void ER_BasicColorMaterial::PrepareForRendering(Rendering::RenderingObject* aObj, int meshIndex)
	{
		auto context = ER_Material::GetGame()->Direct3DDeviceContext();

		assert(aObj);

		ER_Material::PrepareForRendering(aObj, meshIndex);

		mConstantBuffer.Data.Color = XMFLOAT4{0.0, 1.0, 0.0, 0.0};
		mConstantBuffer.ApplyChanges(context);
		ID3D11Buffer* CBs[1] = { mConstantBuffer.Buffer() };

		context->PSSetConstantBuffers(0, 1, CBs);
	}

}