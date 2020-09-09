#include "stdafx.h"

#include "RenderableOBB.h"
#include "Game.h"
#include "GameException.h"
#include "BasicMaterial.h"
#include "VertexDeclarations.h"
#include "ColorHelper.h"
#include "MatrixHelper.h"
#include "VectorHelper.h"
#include "Camera.h"
#include "VertexDeclarations.h"
#include "Utility.h"

namespace Library
{
	RTTI_DEFINITIONS(RenderableOBB)

	const XMVECTORF32 RenderableOBB::DefaultColor = ColorHelper::Green;

	const UINT	 RenderableOBB::OBBVertexCount = 8;
	const UINT	 RenderableOBB::OBBPrimitiveCount = 12;
	const UINT	 RenderableOBB::OBBIndicesPerPrimitive = 2;
	const UINT	 RenderableOBB::OBBIndexCount = OBBPrimitiveCount * OBBIndicesPerPrimitive;
	const USHORT RenderableOBB::OBBIndices[] = {
		// Near plane lines
		0, 1,
		1, 2,
		2, 3,
		3, 0,

		// Sides
		0, 4,
		1, 5,
		2, 6,
		3, 7,

		// Far plane lines
		4, 5,
		5, 6,
		6, 7,
		7, 4
	};
	RenderableOBB::RenderableOBB(Game& game, Camera& camera)
		: DrawableGameComponent(game, camera),
		mVertexBuffer(nullptr), mIndexBuffer(nullptr), mMaterial(nullptr), mPass(nullptr), mInputLayout(nullptr),
		mColor(DefaultColor), mPosition(Vector3Helper::Zero), mDirection(Vector3Helper::Forward), mUp(Vector3Helper::Up), mRight(Vector3Helper::Right),
		mWorldMatrix(MatrixHelper::Identity), mVertices(0), mCornersAABB(0)
	{
	}

	RenderableOBB::RenderableOBB(Game& game, Camera& camera, const XMFLOAT4& color)
		: DrawableGameComponent(game, camera),
		mVertexBuffer(nullptr), mIndexBuffer(nullptr), mMaterial(nullptr), mPass(nullptr), mInputLayout(nullptr),
		mColor(color), mPosition(Vector3Helper::Zero), mDirection(Vector3Helper::Forward), mUp(Vector3Helper::Up), mRight(Vector3Helper::Right),
		mWorldMatrix(MatrixHelper::Identity), mVertices(0), mCornersAABB(0)
	{
	}

	RenderableOBB::~RenderableOBB()
	{
		ReleaseObject(mIndexBuffer);
		ReleaseObject(mVertexBuffer);

		if (mMaterial != nullptr)
		{
			delete mMaterial->GetEffect();
			delete mMaterial;
			mMaterial = nullptr;
		}
	}

	void RenderableOBB::SetPosition(const XMFLOAT3& position)
	{
		mPosition = position;
		mCenter = XMFLOAT3
		(
			mCenterConstant.x + position.x,
			mCenterConstant.y + position.y,
			mCenterConstant.z + position.z
		);

	}

	void RenderableOBB::SetRotationMatrix(const XMMATRIX& rotationMat)
	{
		mTransformMatrix = rotationMat;
		mTranformationMatrix *= rotationMat;
	}

	void RenderableOBB::SetOBB(const std::vector<XMFLOAT3>& aabb)
	{
		mOBB = &aabb;
	}

	void RenderableOBB::InitializeGeometry(const std::vector<XMFLOAT3>& aabb, XMMATRIX matrix)
	{

		mOBB = &aabb;


		XMFLOAT3 Center = XMFLOAT3
		{
			mOBB->at(0).x + (mOBB->at(1).x - mOBB->at(0).x) / 2.0f,
			mOBB->at(0).y + (mOBB->at(1).y - mOBB->at(0).y) / 2.0f,
			mOBB->at(0).z + (mOBB->at(1).z - mOBB->at(0).z) / 2.0f
		};

		mExtents = XMFLOAT3
		{
			mOBB->at(1).x - Center.x,
			mOBB->at(1).y - Center.y,
			mOBB->at(1).z - Center.z
		};
		

		mTranformationMatrix = XMMatrixTranslation(Center.x, Center.y, Center.z);


		//mModifiedAABB = aabb;
		//mModifiedAABB2 = aabb;


		XMFLOAT3 xv = XMFLOAT3(mExtents.x, 0, 0);
		XMFLOAT3 yv = XMFLOAT3(0, mExtents.y, 0);
		XMFLOAT3 zv = XMFLOAT3(0, 0, mExtents.z);


		XMStoreFloat3(&xv, XMVector3TransformNormal(XMLoadFloat3(&xv), mTranformationMatrix));
		XMStoreFloat3(&yv, XMVector3TransformNormal(XMLoadFloat3(&yv), mTranformationMatrix));
		XMStoreFloat3(&zv, XMVector3TransformNormal(XMLoadFloat3(&zv), mTranformationMatrix));
		
		XMVECTOR scale;
		XMVECTOR translation;
		XMVECTOR rotation;
		XMMatrixDecompose(&scale, &rotation, &translation, mTranformationMatrix );


		
		//XMFLOAT3 center;
		XMStoreFloat3(&mCenter, translation);
		XMStoreFloat3(&mCenterConstant, translation);



		//std::vector<XMFLOAT3> vertices;
		mVertices.push_back(XMFLOAT3(mCenter.x + xv.x + yv.x + zv.x, mCenter.y + xv.y + yv.y + zv.y, mCenter.z + xv.z + yv.z + zv.z));
		mVertices.push_back(XMFLOAT3(mCenter.x + xv.x + yv.x - zv.x, mCenter.y + xv.y + yv.y - zv.y, mCenter.z + xv.z + yv.z - zv.z));
		mVertices.push_back(XMFLOAT3(mCenter.x - xv.x + yv.x - zv.x, mCenter.y - xv.y + yv.y - zv.y, mCenter.z - xv.z + yv.z - zv.z));
		mVertices.push_back(XMFLOAT3(mCenter.x - xv.x + yv.x + zv.x, mCenter.y - xv.y + yv.y + zv.y, mCenter.z - xv.z + yv.z + zv.z));
		mVertices.push_back(XMFLOAT3(mCenter.x + xv.x - yv.x + zv.x, mCenter.y + xv.y - yv.y + zv.y, mCenter.z + xv.z - yv.z + zv.z));
		mVertices.push_back(XMFLOAT3(mCenter.x + xv.x - yv.x - zv.x, mCenter.y + xv.y - yv.y - zv.y, mCenter.z + xv.z - yv.z - zv.z));
		mVertices.push_back(XMFLOAT3(mCenter.x - xv.x - yv.x - zv.x, mCenter.y - xv.y - yv.y - zv.y, mCenter.z - xv.z - yv.z - zv.z));
		mVertices.push_back(XMFLOAT3(mCenter.x - xv.x - yv.x + zv.x, mCenter.y - xv.y - yv.y + zv.y, mCenter.z - xv.z - yv.z + zv.z));

		mCornersAABB.push_back(XMFLOAT3(mCenter.x + xv.x + yv.x + zv.x, mCenter.y + xv.y + yv.y + zv.y, mCenter.z + xv.z + yv.z + zv.z));
		mCornersAABB.push_back(XMFLOAT3(mCenter.x + xv.x + yv.x - zv.x, mCenter.y + xv.y + yv.y - zv.y, mCenter.z + xv.z + yv.z - zv.z));
		mCornersAABB.push_back(XMFLOAT3(mCenter.x - xv.x + yv.x - zv.x, mCenter.y - xv.y + yv.y - zv.y, mCenter.z - xv.z + yv.z - zv.z));
		mCornersAABB.push_back(XMFLOAT3(mCenter.x - xv.x + yv.x + zv.x, mCenter.y - xv.y + yv.y + zv.y, mCenter.z - xv.z + yv.z + zv.z));
		mCornersAABB.push_back(XMFLOAT3(mCenter.x + xv.x - yv.x + zv.x, mCenter.y + xv.y - yv.y + zv.y, mCenter.z + xv.z - yv.z + zv.z));
		mCornersAABB.push_back(XMFLOAT3(mCenter.x + xv.x - yv.x - zv.x, mCenter.y + xv.y - yv.y - zv.y, mCenter.z + xv.z - yv.z - zv.z));
		mCornersAABB.push_back(XMFLOAT3(mCenter.x - xv.x - yv.x - zv.x, mCenter.y - xv.y - yv.y - zv.y, mCenter.z - xv.z - yv.z - zv.z));
		mCornersAABB.push_back(XMFLOAT3(mCenter.x - xv.x - yv.x + zv.x, mCenter.y - xv.y - yv.y + zv.y, mCenter.z - xv.z - yv.z + zv.z));



		InitializeVertexBuffer(mVertices);
	}

	void RenderableOBB::Initialize()
	{
		Effect* effect = new Effect(*mGame);
		effect->CompileFromFile(Utility::GetFilePath(L"content\\effects\\BasicEffect.fx"));

		mMaterial = new BasicMaterial();
		mMaterial->Initialize(effect);

		mPass = mMaterial->CurrentTechnique()->Passes().at(0);
		mInputLayout = mMaterial->InputLayouts().at(mPass);

		InitializeIndexBuffer();
	}

	void RenderableOBB::Update(const GameTime& gameTime)
	{
		TransformOBB(gameTime);

		XMMATRIX worldMatrix = XMMatrixIdentity();
		MatrixHelper::SetForward(worldMatrix, mDirection);
		MatrixHelper::SetUp(worldMatrix, mUp);
		MatrixHelper::SetRight(worldMatrix, mRight);
		MatrixHelper::SetTranslation(worldMatrix, mPosition);

		XMStoreFloat4x4(&mWorldMatrix, worldMatrix);
	}

	void RenderableOBB::Draw(const GameTime& gameTime)
	{
		assert(mPass != nullptr);
		assert(mInputLayout != nullptr);

		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout);

		UINT stride = sizeof(VertexPositionColor);
		UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
		direct3DDeviceContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

		XMMATRIX world = XMLoadFloat4x4(&mWorldMatrix);
		XMMATRIX wvp = world * mCamera->ViewMatrix() * mCamera->ProjectionMatrix();
		mMaterial->WorldViewProjection() << wvp;

		mPass->Apply(0, direct3DDeviceContext);

		direct3DDeviceContext->DrawIndexed(OBBIndexCount, 0, 0);
	}
	void RenderableOBB::SetColor(XMFLOAT4 color)
	{
		mColor = color;
	}

	void RenderableOBB::UpdateColor(XMFLOAT4 color)
	{

		VertexPositionColor vertices[OBBVertexCount];

		for (UINT i = 0; i < OBBVertexCount; i++)
		{
			vertices[i].Position = XMFLOAT4(mVertices[i].x, mVertices[i].y, mVertices[i].z, 1.0f);
			vertices[i].Color = color;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
		mGame->Direct3DDeviceContext()->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		memcpy(mappedResource.pData, vertices, sizeof(VertexPositionColor) * OBBVertexCount);
		mGame->Direct3DDeviceContext()->Unmap(mVertexBuffer, 0);

	}

	void RenderableOBB::TransformOBB(const GameTime& gameTime)
	{
		if (mVertices.size() != 8 ) return;
		XMStoreFloat3(&mVertices.at(0), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(0)), mTransformMatrix));
		XMStoreFloat3(&mVertices.at(1), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(1)), mTransformMatrix));
		XMStoreFloat3(&mVertices.at(2), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(2)), mTransformMatrix));
		XMStoreFloat3(&mVertices.at(3), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(3)), mTransformMatrix));
		XMStoreFloat3(&mVertices.at(4), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(4)), mTransformMatrix));
		XMStoreFloat3(&mVertices.at(5), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(5)), mTransformMatrix));
		XMStoreFloat3(&mVertices.at(6), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(6)), mTransformMatrix));
		XMStoreFloat3(&mVertices.at(7), XMVector3Transform(XMLoadFloat3(&mCornersAABB.at(7)), mTransformMatrix));
	}


	void RenderableOBB::InitializeVertexBuffer(const std::vector<XMFLOAT3>& aabb)
	{
		ReleaseObject(mVertexBuffer);

		VertexPositionColor vertices[OBBVertexCount];
		//const XMFLOAT3* corners = frustum.Corners();
		for (UINT i = 0; i < OBBVertexCount; i++)
		{
			vertices[i].Position = XMFLOAT4(aabb[i].x, aabb[i].y, aabb[i].z, 1.0f);
			vertices[i].Color = mColor;
		}

		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
		vertexBufferDesc.ByteWidth = sizeof(VertexPositionColor) * OBBVertexCount;
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData;
		ZeroMemory(&vertexSubResourceData, sizeof(vertexSubResourceData));
		vertexSubResourceData.pSysMem = vertices;
		if (FAILED(mGame->Direct3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, &mVertexBuffer)))
		{
			throw GameException("ID3D11Device::CreateBuffer() failed.");
		}
	}

	void RenderableOBB::InitializeIndexBuffer()
	{
		D3D11_BUFFER_DESC indexBufferDesc;
		ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
		indexBufferDesc.ByteWidth = sizeof(USHORT) * OBBIndexCount;
		indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA indexSubResourceData;
		ZeroMemory(&indexSubResourceData, sizeof(indexSubResourceData));
		indexSubResourceData.pSysMem = OBBIndices;
		if (FAILED(mGame->Direct3DDevice()->CreateBuffer(&indexBufferDesc, &indexSubResourceData, &mIndexBuffer)))
		{
			throw GameException("ID3D11Device::CreateBuffer() failed.");
		}
	}
}