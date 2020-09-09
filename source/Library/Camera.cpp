#include "stdafx.h"

#include "Camera.h"
#include "Game.h"
#include "GameTime.h"
#include "VectorHelper.h"
#include "MatrixHelper.h"
#include "RenderingObject.h"
#include "Utility.h"

#define MAX_NUM_CASCADES 3

namespace Library
{
	RTTI_DEFINITIONS(Camera)

	const float Camera::DefaultFieldOfView = XM_PIDIV2;
	const float Camera::DefaultNearPlaneDistance = 0.01f;
	const float Camera::DefaultFarPlaneDistance = 600;

	const float cascadeDistances[MAX_NUM_CASCADES] = { 100.0f, 500.0f, 1200.0f };

	Camera::Camera(Game& game)
		: GameComponent(game),
		mFieldOfView(DefaultFieldOfView), mAspectRatio(game.AspectRatio()), mNearPlaneDistance(DefaultNearPlaneDistance), mFarPlaneDistance(DefaultFarPlaneDistance),
		mPosition(), mDirection(), mUp(), mRight(), mViewMatrix(), mProjectionMatrix(), mFrustum(XMMatrixIdentity())
	{
	}

	Camera::Camera(Game& game, float fieldOfView, float aspectRatio, float nearPlaneDistance, float farPlaneDistance)
		: GameComponent(game),
		mFieldOfView(fieldOfView), mAspectRatio(aspectRatio), mNearPlaneDistance(nearPlaneDistance), mFarPlaneDistance(farPlaneDistance),
		mPosition(), mDirection(), mUp(), mRight(), mViewMatrix(), mProjectionMatrix(), mFrustum(XMMatrixIdentity())
	{
	}

	Camera::~Camera()
	{
	}

	const XMFLOAT3& Camera::Position() const
	{
		return mPosition;
	}

	const XMFLOAT3& Camera::Direction() const
	{
		return mDirection;
	}

	const XMFLOAT3& Camera::Up() const
	{
		return mUp;
	}

	const XMFLOAT3& Camera::Right() const
	{
		return mRight;
	}

	XMVECTOR Camera::PositionVector() const
	{
		return XMLoadFloat3(&mPosition);
	}

	XMVECTOR Camera::DirectionVector() const
	{
		return XMLoadFloat3(&mDirection);
	}

	XMVECTOR Camera::UpVector() const
	{
		return XMLoadFloat3(&mUp);
	}

	XMVECTOR Camera::RightVector() const
	{
		return XMLoadFloat3(&mRight);
	}

	float Camera::AspectRatio() const
	{
		return mAspectRatio;
	}

	float Camera::FieldOfView() const
	{
		return mFieldOfView;
	}

	float Camera::NearPlaneDistance() const
	{
		return mNearPlaneDistance;
	}

	float Camera::FarPlaneDistance() const
	{
		return mFarPlaneDistance;
	}

	XMMATRIX Camera::ViewMatrix() const
	{
		return XMLoadFloat4x4(&mViewMatrix);
	}	
	XMFLOAT4X4 Camera::ViewMatrix4X4() const
	{
		return mViewMatrix;
	}

	XMMATRIX Camera::ProjectionMatrix() const
	{
		return XMLoadFloat4x4(&mProjectionMatrix);
	}

	XMFLOAT4X4 Camera::ProjectionMatrix4X4() const
	{
		return mProjectionMatrix;
	}

	XMMATRIX Camera::ViewProjectionMatrix() const
	{
		XMMATRIX viewMatrix = XMLoadFloat4x4(&mViewMatrix);
		XMMATRIX projectionMatrix = XMLoadFloat4x4(&mProjectionMatrix);

		return XMMatrixMultiply(viewMatrix, projectionMatrix);
	}

	void Camera::SetPosition(FLOAT x, FLOAT y, FLOAT z)
	{
		XMVECTOR position = XMVectorSet(x, y, z, 1.0f);
		SetPosition(position);
	}

	void Camera::SetPosition(FXMVECTOR position)
	{
		XMStoreFloat3(&mPosition, position);
	}

	void Camera::SetPosition(const XMFLOAT3& position)
	{
		mPosition = position;
	}
	void Camera::SetDirection(const XMFLOAT3& direction)
	{
		mDirection = direction;
	}
	void Camera::SetFOV(float fov)
	{
		mFieldOfView = fov;
		UpdateProjectionMatrix();

	}

	void Camera::SetFarPlaneDistance(float value)
	{
		mFarPlaneDistance = value;
		UpdateProjectionMatrix();
	}

	XMMATRIX Camera::GetCustomViewProjectionMatrixForCascade(int cascadeIndex)
	{
		XMMATRIX projectionMatrix;
		float delta = 5.0f;

		switch (cascadeIndex)
		{
		case 0:
			projectionMatrix = XMMatrixPerspectiveFovRH(mFieldOfView, mAspectRatio, mNearPlaneDistance, cascadeDistances[0]);
			break;
		case 1:
			projectionMatrix = XMMatrixPerspectiveFovRH(mFieldOfView, mAspectRatio, cascadeDistances[0], cascadeDistances[1]);
			break;
		case 2:
			projectionMatrix = XMMatrixPerspectiveFovRH(mFieldOfView, mAspectRatio, cascadeDistances[1], cascadeDistances[2]);
			break;


		default:
			projectionMatrix = XMMatrixPerspectiveFovRH(mFieldOfView, mAspectRatio, mNearPlaneDistance, cascadeDistances[2]);
		}

		XMMATRIX viewMatrix = XMLoadFloat4x4(&mViewMatrix);

		return XMMatrixMultiply(viewMatrix, projectionMatrix);
	}

	void Camera::SetNearPlaneDistance(float value)
	{
		mNearPlaneDistance = value;
		UpdateProjectionMatrix();
	}

	void Camera::Reset()
	{
		mPosition = Vector3Helper::Zero;
		mDirection = Vector3Helper::Forward;
		mUp = Vector3Helper::Up;
		mRight = Vector3Helper::Right;

		UpdateViewMatrix();
	}

	void Camera::Initialize()
	{
		UpdateProjectionMatrix();
		Reset();

		mFrustum.SetMatrix(ViewProjectionMatrix());
	}

	void Camera::Update(const GameTime& gameTime)
	{
		UpdateViewMatrix();
		mFrustum.SetMatrix(ViewProjectionMatrix());
	}

	void Camera::UpdateViewMatrix()
	{
		XMVECTOR eyePosition = XMLoadFloat3(&mPosition);
		XMVECTOR direction = XMLoadFloat3(&mDirection);
		XMVECTOR upDirection = XMLoadFloat3(&mUp);

		XMMATRIX viewMatrix = XMMatrixLookToRH(eyePosition, direction, upDirection);
		XMStoreFloat4x4(&mViewMatrix, viewMatrix);
	}

	void Camera::UpdateProjectionMatrix()
	{
		XMMATRIX projectionMatrix = XMMatrixPerspectiveFovRH(mFieldOfView, mAspectRatio, mNearPlaneDistance, mFarPlaneDistance);
		XMStoreFloat4x4(&mProjectionMatrix, projectionMatrix);
	}

	void Camera::ApplyRotation(CXMMATRIX transform)
	{
		XMVECTOR direction = XMLoadFloat3(&mDirection);
		XMVECTOR up = XMLoadFloat3(&mUp);

		direction = XMVector3TransformNormal(direction, transform);
		direction = XMVector3Normalize(direction);

		up = XMVector3TransformNormal(up, transform);
		up = XMVector3Normalize(up);

		XMVECTOR right = XMVector3Cross(direction, up);
		up = XMVector3Cross(right, direction);

		XMStoreFloat3(&mDirection, direction);
		XMStoreFloat3(&mUp, up);
		XMStoreFloat3(&mRight, right);

		rotationMatrix = transform;
	}

	void Camera::ApplyRotation(const XMFLOAT4X4& transform)
	{
		XMMATRIX transformMatrix = XMLoadFloat4x4(&transform);
		ApplyRotation(transformMatrix);

	}

	XMMATRIX Camera::RotationTransformMatrix() const
	{
		return rotationMatrix;
	}

	void Camera::Cull(const std::map<std::string, Rendering::RenderingObject*> objects)
	{
		if (!Utility::IsCameraCulling)
		{
			for (auto object : objects) 
			{
				if (object.second->IsInstanced())
					object.second->UpdateInstanceBuffer((object.second->GetInstancesData()));
				else
					object.second->Visible(true);
			}
			
			return;
		}

		for (auto object : objects)
		{
			bool isInstanced = object.second->IsInstanced();
			std::vector<Rendering::InstancedData> newInstanceData;
			int instanceCount = object.second->GetInstanceCount();

			for (int i = 0; i < ((isInstanced) ? (instanceCount) : 1); i++)
			{
				bool cull = false;
				std::vector<XMFLOAT3> aabb = object.second->GetAABB();
				XMFLOAT3 position; 
				MatrixHelper::GetTranslation(object.second->GetTransformationMatrix(), position);
				XMMATRIX instanceWorldMatrix = XMMatrixIdentity();
				
				if (isInstanced)
				{
					instanceWorldMatrix = XMLoadFloat4x4(&(object.second->GetInstancesData()[i].World));
					MatrixHelper::GetTranslation(instanceWorldMatrix, position);
				}

				// start a loop through all frustum planes
				for (int planeID = 0; planeID < 6; ++planeID)
				{
					XMVECTOR planeNormal = XMVectorSet(mFrustum.Planes()[planeID].x, mFrustum.Planes()[planeID].y, mFrustum.Planes()[planeID].z, 0.0f);
					float planeConstant = mFrustum.Planes()[planeID].w;

					XMFLOAT3 axisVert;

					// x-axis
					if (mFrustum.Planes()[planeID].x > 0.0f)
						axisVert.x = aabb[0].x + position.x;
					else
						axisVert.x = aabb[1].x + position.x;

					// y-axis
					if (mFrustum.Planes()[planeID].y > 0.0f)
						axisVert.y = aabb[0].y + position.y;
					else
						axisVert.y = aabb[1].y + position.y;

					// z-axis
					if (mFrustum.Planes()[planeID].z > 0.0f)
						axisVert.z = aabb[0].z + position.z;
					else
						axisVert.z = aabb[1].z + position.z;


					if (XMVectorGetX(XMVector3Dot(planeNormal, XMLoadFloat3(&axisVert))) + planeConstant > 0.0f)
					{
						cull = true;
						object.second->Visible(true);
						// Skip remaining planes to check and move on 


						break;
					}
				}
				
				if (!isInstanced)
					object.second->Visible(!cull);
				else if (!cull)
					newInstanceData.push_back(instanceWorldMatrix);

			}

			if (isInstanced)
				object.second->UpdateInstanceBuffer(newInstanceData);
		
		}

	}

	float Camera::GetCameraCascadeDistance(int index)
	{
		assert(index < (sizeof(cascadeDistances) / sizeof(cascadeDistances[0])));
		return cascadeDistances[index];
	}

}