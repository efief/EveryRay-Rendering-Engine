#pragma once
#include "Common.h"
#include "Game.h"
#include "Camera.h"
#include "GameTime.h"
#include "CustomRenderTarget.h"

namespace Library
{
	class Effect;
	class FullScreenRenderTarget;
	class FullScreenQuad;
}

namespace Rendering
{
	class ColorFilterMaterial;
	class VignetteMaterial;
	class ColorGradingMaterial;
	class MotionBlurMaterial;
	class FXAAMaterial;
	class ScreenSpaceReflectionsMaterial;

	namespace EffectElements
	{
		struct TonemapEffect
		{
			TonemapEffect():
				CalcLumPS(nullptr),
				AvgLumPS(nullptr),
				BrightPS(nullptr),
				AddPS(nullptr),
				BlurHPS(nullptr),
				BlurVPS(nullptr),
				ToneMapWithBloomPS(nullptr),
				EmptyPassPS(nullptr)
			{
			}

			~TonemapEffect()
			{
				DeleteObject(LuminanceResource);
				DeleteObject(AvgLuminanceResource);
				DeleteObject(BrightResource);
				DeleteObject(BlurHorizontalResource);
				DeleteObject(BlurVerticalResource);
				DeleteObject(BlurSummedResource);

				ReleaseObject(BloomConstants);
				ReleaseObject(ConstBuffer);
				ReleaseObject(LinearSampler);

				ReleaseObject(CalcLumPS);
				ReleaseObject(AvgLumPS);
				ReleaseObject(BrightPS);
				ReleaseObject(AddPS);
				ReleaseObject(BlurHPS);
				ReleaseObject(BlurVPS);
				ReleaseObject(ToneMapWithBloomPS);
				ReleaseObject(EmptyPassPS);
			}

			CustomRenderTarget* LuminanceResource = nullptr;
			CustomRenderTarget* AvgLuminanceResource = nullptr;
			CustomRenderTarget* BrightResource = nullptr;
			CustomRenderTarget* BlurHorizontalResource = nullptr;
			CustomRenderTarget* BlurVerticalResource = nullptr;
			CustomRenderTarget* BlurSummedResource = nullptr;

			ID3D11PixelShader* CalcLumPS = nullptr;
			ID3D11PixelShader* AvgLumPS = nullptr;
			ID3D11PixelShader* BrightPS = nullptr;
			ID3D11PixelShader* AddPS = nullptr;
			ID3D11PixelShader* BlurHPS = nullptr;
			ID3D11PixelShader* BlurVPS = nullptr;
			ID3D11PixelShader* ToneMapWithBloomPS = nullptr;
			ID3D11PixelShader* EmptyPassPS = nullptr;

			ID3D11Buffer* BloomConstants = nullptr;
			ID3D11Buffer* ConstBuffer = nullptr;
			ID3D11SamplerState* LinearSampler = nullptr;

			float middlegrey = 0.053f;
			float bloomthreshold = 0.342f;
			float bloommultiplier = 0.837f;
			float luminanceWeights[3] = { 0.27f, 0.67f, 0.06f };
			bool isActive = true;
		};
		
		struct VignetteEffect
		{
			VignetteEffect() :
				Material(nullptr), Quad(nullptr), OutputTexture(nullptr)
			{}

			~VignetteEffect()
			{
				DeleteObject(Material);
				DeleteObject(Quad);
				ReleaseObject(OutputTexture);
			}

			VignetteMaterial* Material;
			FullScreenQuad* Quad;
			ID3D11ShaderResourceView* OutputTexture;

			bool isActive = false;

			float radius = 0.75f;
			float softness = 0.5f;
		};

		struct FXAAEffect
		{
			FXAAEffect() :
				Material(nullptr), Quad(nullptr), OutputTexture(nullptr)
			{}

			~FXAAEffect()
			{
				DeleteObject(Material);
				DeleteObject(Quad);
				ReleaseObject(OutputTexture);
			}

			FXAAMaterial* Material;
			FullScreenQuad* Quad;
			ID3D11ShaderResourceView* OutputTexture;

			bool isActive = false;
		};

		struct ColorGradingEffect
		{
			ColorGradingEffect() :
				Material(nullptr), Quad(nullptr), OutputTexture(nullptr),
				LUT1(nullptr),LUT2(nullptr),LUT3(nullptr)
			{}

			~ColorGradingEffect()
			{
				DeleteObject(Material);
				DeleteObject(Quad);
				ReleaseObject(LUT1);
				ReleaseObject(LUT2);
				ReleaseObject(LUT3);
				ReleaseObject(OutputTexture);

			}

			ColorGradingMaterial* Material;
			FullScreenQuad* Quad;
			ID3D11ShaderResourceView* OutputTexture;

			ID3D11ShaderResourceView* LUT1;
			ID3D11ShaderResourceView* LUT2;
			ID3D11ShaderResourceView* LUT3;

			bool isActive = true;
			int currentLUT = 0;

		};

		struct SSREffect
		{
			SSREffect() :
				Material(nullptr), Quad(nullptr), InputColor(nullptr)
			{}

			~SSREffect()
			{
				DeleteObject(Material);
				DeleteObject(Quad);
				ReleaseObject(InputColor);

			}

			ScreenSpaceReflectionsMaterial* Material;
			FullScreenQuad* Quad;
			ID3D11ShaderResourceView* InputColor;

			bool isActive = true;
			int rayCount = 50;
			float stepSize = 0.741f;
			float maxThickness = 0.00021f;
		};

		struct MotionBlurEffect
		{
			MotionBlurEffect() :
				Material(nullptr), Quad(nullptr), OutputTexture(nullptr),
				DepthMap(nullptr)
			{}

			~MotionBlurEffect()
			{
				DeleteObject(Material);
				DeleteObject(Quad);
				ReleaseObject(DepthMap);
				ReleaseObject(OutputTexture);

			}

			MotionBlurMaterial* Material;
			FullScreenQuad* Quad;
			ID3D11ShaderResourceView* OutputTexture;
			ID3D11ShaderResourceView* DepthMap;

			XMMATRIX WVP = XMMatrixIdentity();
			XMMATRIX preWVP = XMMatrixIdentity();
			XMMATRIX WIT = XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixIdentity()));

			bool isActive = false;

			float amount = 17.0f;
		};

	}

	class PostProcessingStack
	{
	public:

		PostProcessingStack() = delete;
		PostProcessingStack(Game& pGame, Camera& pCamera);
		~PostProcessingStack();

		void UpdateVignetteMaterial();
		void UpdateColorGradingMaterial();
		void UpdateMotionBlurMaterial();
		void UpdateFXAAMaterial();
		void UpdateSSRMaterial(ID3D11ShaderResourceView* normal, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* extra, float time);

		void Initialize(bool pTonemap, bool pMotionBlur, bool pColorGrading, bool pVignette, bool pFXAA, bool pSSR = true);
		void Begin(bool clear = true);
		void End();
		void BeginRenderingToExtraRT(bool clear);
		void EndRenderingToExtraRT();
		void SetMainRT(ID3D11ShaderResourceView* srv);
		void ResetMainRTtoOriginal();

		void DrawEffects(const GameTime & gameTime);

		void Update();
		void DrawFullscreenQuad(ID3D11DeviceContext* pContext);
		void ResetOMToMainRenderTarget();
		void ShowPostProcessingWindow();

		ID3D11ShaderResourceView* GetDepthOutputTexture();
		ID3D11ShaderResourceView* GetPrepassColorOutputTexture();
		ID3D11ShaderResourceView * GetExtraColorOutputTexture();

		bool isWindowOpened = false;

	private:
		void UpdateTonemapConstantBuffer(ID3D11DeviceContext * pD3DImmediateContext, ID3D11Buffer* buffer, int mipLevel0, int mipLevel1, unsigned int width, unsigned int height);
		void ComputeLuminance(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* pInput, ID3D11RenderTargetView* pOutput);
		void ComputeBrightPass(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* pInput, ID3D11RenderTargetView* pOutput);
		void ComputeHorizontalBlur(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* pInput, ID3D11RenderTargetView* pOutput);
		void ComputeVerticalBlur(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* pInput, ID3D11RenderTargetView* pOutput);
		void ComputeToneMapWithBloom(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* pInput, ID3D11ShaderResourceView* pAVG, ID3D11ShaderResourceView* pBloom, ID3D11RenderTargetView* pOutput);
		void PerformEmptyPass(ID3D11DeviceContext * pContext, ID3D11ShaderResourceView * pInput, ID3D11RenderTargetView * pOutput);

		Game& game;
		Camera& camera;

		EffectElements::VignetteEffect* mVignetteEffect;
		EffectElements::ColorGradingEffect* mColorGradingEffect;
		EffectElements::MotionBlurEffect* mMotionBlurEffect;
		EffectElements::FXAAEffect* mFXAAEffect;
		EffectElements::TonemapEffect* mTonemapEffect;
		EffectElements::SSREffect* mSSREffect;
		
		FullScreenRenderTarget* mMainRenderTarget;
		FullScreenRenderTarget* mVignetteRenderTarget;
		FullScreenRenderTarget* mColorGradingRenderTarget;
		FullScreenRenderTarget* mMotionBlurRenderTarget;
		FullScreenRenderTarget* mFXAARenderTarget;
		FullScreenRenderTarget* mTonemapRenderTarget;
		FullScreenRenderTarget* mSSRRenderTarget;

		FullScreenRenderTarget* mExtraRenderTarget;

		ID3D11Buffer* mQuadVB;
		ID3D11VertexShader* mFullScreenQuadVS;
		ID3D11InputLayout* mFullScreenQuadLayout;

		ID3D11ShaderResourceView* mOriginalMainRTSRV = nullptr;

		bool mVignetteLoaded = false;
		bool mColorGradingLoaded = false;
		bool mMotionBlurLoaded = false;
		bool mFXAALoaded = false;
		bool mSSRLoaded = false;

	};
}