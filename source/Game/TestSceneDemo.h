#pragma once

#include "..\Library\DrawableGameComponent.h"
#include "..\Library\DemoLevel.h"
#include "..\Library\ConstantBuffer.h"
#include "..\Library\Frustum.h"

using namespace Library;

namespace Library
{
	class Effect;
	class Keyboard;
	class Light;
	class RenderStateHelper;
	class DepthMap;
	class DirectionalLight;
	class Skybox;
	class Grid;
	class IBLRadianceMap;
	class Frustum;
	class FullScreenRenderTarget;
	class FullScreenQuad;
	class GBuffer;
	class ShadowMapper;
	class Terrain;
	class Foliage;
	class Scene;
}

namespace DirectX
{
	class SpriteBatch;
	class SpriteFont;
}

namespace Rendering
{
	class RenderingObject;
	class PostProcessingStack;

	namespace VolumetricCloudsData {
		struct FrameCB
		{
			XMMATRIX	invProj;
			XMMATRIX	invView;
			XMVECTOR	lightDir;
			XMVECTOR	cameraPos;
		};
		struct CloudsCB
		{
			float Time;
			float Crispiness;
			float Coverage;
			float Speed;
		};
	}

	class TestSceneDemo : public DrawableGameComponent, public DemoLevel
	{
		RTTI_DECLARATIONS(TestSceneDemo, DrawableGameComponent)

	public:
		TestSceneDemo(Game& game, Camera& camera, Editor& editor);
		~TestSceneDemo();

		virtual void Create() override;
		virtual void Destroy() override;
		virtual bool IsComponent() override;
		virtual void UpdateLevel(const GameTime& gameTime) override;
		virtual void DrawLevel(const GameTime& gameTime) override;

	private:
		TestSceneDemo();
		TestSceneDemo(const TestSceneDemo& rhs);
		TestSceneDemo& operator=(const TestSceneDemo& rhs);

		void DrawVolumetricClouds(const GameTime& gameTime);
		void UpdateStandardLightingPBRMaterialVariables(const std::string& objectName, int meshIndex);
		void UpdateDeferredPrepassMaterialVariables(const std::string& objectName, int meshIndex);
		void UpdateShadow0MaterialVariables(const std::string & objectName, int meshIndex);
		void UpdateShadow1MaterialVariables(const std::string & objectName, int meshIndex);
		void UpdateShadow2MaterialVariables(const std::string & objectName, int meshIndex); 
		void UpdateImGui();
		void Initialize();

		ID3D11PixelShader* VCMainPS;
		Scene* mScene;
		Keyboard* mKeyboard;
		XMFLOAT4X4 mWorldMatrix;
		RenderStateHelper* mRenderStateHelper;
		Skybox* mSkybox;
		Grid* mGrid;
		ShadowMapper* mShadowMapper;
		FullScreenQuad* mSSRQuad;
		DirectionalLight* mDirectionalLight;
		GBuffer* mGBuffer;
		PostProcessingStack* mPostProcessingStack;
		std::vector<Foliage*> mFoliageCollection;

		ID3D11ShaderResourceView* mIrradianceTextureSRV;
		ID3D11ShaderResourceView* mRadianceTextureSRV;
		ID3D11ShaderResourceView* mIntegrationMapTextureSRV;
		std::unique_ptr<IBLRadianceMap> mIBLRadianceMap;

		float mWindStrength = 1.0f;
		float mWindFrequency = 1.0f;
		float mWindGustDistance = 1.0f;
		//Terrain* mTerrain;


		ID3D11ShaderResourceView* mCloudTextureSRV = nullptr;
		ID3D11ShaderResourceView* mWeatherTextureSRV = nullptr;

		ID3D11SamplerState* mCloudSS = nullptr;
		ID3D11SamplerState* mWeatherSS = nullptr;

		float mCloudsCrispiness = 40.0f;
		float mCloudsCoverage = 0.5f;
		float mCloudsSpeed = 1000;

		ConstantBuffer<VolumetricCloudsData::FrameCB> mVolumetricCloudsFrameConstantBuffer;
		ConstantBuffer<VolumetricCloudsData::CloudsCB> mVolumetricCloudsCloudsConstantBuffer;
	};
}