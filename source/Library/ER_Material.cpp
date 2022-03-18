#include "stdafx.h"

#include "ER_Material.h"
#include "Game.h"
#include "GameException.h"
#include "Model.h"
#include "RenderingObject.h"
#include "ShaderCompiler.h"
#include "Utility.h"

namespace Library
{
	static const std::string vertexShaderModel = "vs_5_0";
	static const std::string pixelShaderModel = "ps_5_0";
	static const std::string hullShaderModel = "hs_5_0";
	static const std::string domainShaderModel = "ds_5_0";

	ER_Material::ER_Material(Game& game, unsigned int shaderFlags)
		: GameComponent(game),
		mShaderFlags(shaderFlags)
	{
	}

	ER_Material::~ER_Material()
	{
		ReleaseObject(mInputLayout);
		ReleaseObject(mVS);
		ReleaseObject(mPS);
		ReleaseObject(mGS);
		ReleaseObject(mHS);
		ReleaseObject(mDS);
	}

	// Setting up the pipeline before the draw call
	void ER_Material::PrepareForRendering(Rendering::RenderingObject* aObj, int meshIndex)
	{
		ID3D11DeviceContext* context = GetGame()->Direct3DDeviceContext();

		context->IASetInputLayout(mInputLayout);

		if (mShaderFlags & HAS_VERTEX_SHADER)
			context->VSSetShader(mVS, NULL, 0);
		if (mShaderFlags & HAS_GEOMETRY_SHADER)
			context->GSSetShader(mGS, NULL, 0);
		if (mShaderFlags & HAS_TESSELLATION_SHADER)
		{
			context->DSSetShader(mDS, NULL, 0);	
			context->HSSetShader(mHS, NULL, 0);	
		}
		if (mShaderFlags & HAS_PIXEL_SHADER)
			context->PSSetShader(mPS, NULL, 0);

		//override: apply constant buffers
		//override: set constant buffers in context
		//override: set samplers in context
		//override: set resources in context
	}

	void ER_Material::CreateVertexShader(const std::string& path, D3D11_INPUT_ELEMENT_DESC* inputElementDescriptions, UINT inputElementDescriptionCount)
	{
		std::string compilerErrorMessage = "Failed to compile VSMain from shader: " + path;
		std::string createErrorMessage = "Failed to create vertex shader from shader: " + path;

		ID3DBlob* blob = nullptr;
		if (FAILED(ShaderCompiler::CompileShader(Utility::GetFilePath(Utility::ToWideString(path)).c_str(), "VSMain", vertexShaderModel.c_str(), &blob)))
			throw GameException(compilerErrorMessage.c_str());
		if (FAILED(GetGame()->Direct3DDevice()->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mVS)))
			throw GameException(createErrorMessage.c_str());

		CreateInputLayout(inputElementDescriptions, inputElementDescriptionCount, blob->GetBufferPointer(), blob->GetBufferSize());

		blob->Release();
	}

	void ER_Material::CreatePixelShader(const std::string& path)
	{
		std::string compilerErrorMessage = "Failed to compile PSMain from shader: " + path;
		std::string createErrorMessage = "Failed to create pixel shader from shader: " + path;

		ID3DBlob* blob = nullptr;
		if (FAILED(ShaderCompiler::CompileShader(Utility::GetFilePath(Utility::ToWideString(path)).c_str(), "PSMain", pixelShaderModel.c_str(), &blob)))
			throw GameException(compilerErrorMessage.c_str());
		if (FAILED(GetGame()->Direct3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mPS)))
			throw GameException(createErrorMessage.c_str());
		blob->Release();
	}

	void ER_Material::CreateGeometryShader(const std::string& path)
	{
		//TODO
	}

	void ER_Material::CreateTessellationShader(const std::string& path)
	{
		//TODO
	}

	void ER_Material::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* inputElementDescriptions, UINT inputElementDescriptionCount, const void* shaderBytecodeWithInputSignature, UINT byteCodeLength)
	{
		HRESULT hr = GetGame()->Direct3DDevice()->CreateInputLayout(inputElementDescriptions, inputElementDescriptionCount, shaderBytecodeWithInputSignature, byteCodeLength, &mInputLayout);
		if (FAILED(hr))
			throw GameException("CreateInputLayout() failed when creating material's vertex shader.", hr);
	}
}