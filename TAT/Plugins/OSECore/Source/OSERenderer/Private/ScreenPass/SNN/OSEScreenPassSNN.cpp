// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "ScreenPass/SNN/OSEScreenPassSNN.h"
#include "OSERenderSubsystem.h"

// UE4
#include "Runtime/RenderCore/Public/RenderGraphUtils.h"
#include "Runtime/Renderer/Private/PostProcess/PostProcessing.h"
#include "Runtime/Renderer/Private/BlueNoise.h"
#include "DataDrivenShaderPlatformInfo.h"


//--------------------------------------------------------------------------------------------------
/// SNN console variables
//--------------------------------------------------------------------------------------------------

namespace SNN
{
   // Show flag
   TCustomShowFlag<EShowFlagShippingValue::ForceEnabled> ShowFlag(
      TEXT("OSE_SNN_SF"),                                                     // Name
      true,                                                                   // DefaultEnabled
      SFG_PostProcess,                                                        // Group
      NSLOCTEXT("OSE", "OSE_SNN_SF", "[OSE] SNN (Painterly Post Process)"));  // DisplayName

   namespace CVar
   {
      TAutoConsoleVariable<int32> Enabled(
         TEXT("OSE.SNN.Enabled"),
         0,
         TEXT("Enables or disables the SNN painterly post process\n")
         TEXT(" 0: disabled (default)\n")
         TEXT(" 1: enabled"),
         ECVF_RenderThreadSafe);

      TAutoConsoleVariable<int32> NearBlend(
         TEXT("OSE.SNN.NearBlend"),
         1,
         TEXT("Enables or disables the SNN near distance blend\n")
         TEXT(" 0: disabled\n")
         TEXT(" 1: enabled (default)"),
         ECVF_RenderThreadSafe);

      TAutoConsoleVariable<float> NearBlendMin(
         TEXT("OSE.SNN.NearBlend.Min"),
         1000,
         TEXT("The distance (cm) where the SNN near blend transition begins")
         TEXT(" (no SNN closer than this distance)"),
         ECVF_RenderThreadSafe);

      TAutoConsoleVariable<float> NearBlendMax(
         TEXT("OSE.SNN.NearBlend.Max"),
         2000,
         TEXT("The distance (cm) where the SNN near blend transition ends")
         TEXT(" (full strength at this distance)"),
         ECVF_RenderThreadSafe);

      TAutoConsoleVariable<int32> FarBlend(
         TEXT("OSE.SNN.FarBlend"),
         1,
         TEXT("Enables or disables the SNN far distance blend\n")
         TEXT(" 0: disabled\n")
         TEXT(" 1: enabled (default)"),
         ECVF_RenderThreadSafe);

      TAutoConsoleVariable<float> FarBlendMin(
         TEXT("OSE.SNN.FarBlend.Min"),
         2000,
         TEXT("The distance (cm) where the SNN far blend transition begins")
         TEXT(" (no SNN closer than this distance)"),
         ECVF_RenderThreadSafe);

      TAutoConsoleVariable<float> FarBlendMax(
         TEXT("OSE.SNN.FarBlend.Max"),
         4000,
         TEXT("The distance (cm) where the SNN far blend transition ends")
         TEXT(" (full strength at this distance)"),
         ECVF_RenderThreadSafe);
   }
}


//--------------------------------------------------------------------------------------------------
/// SNN texture parameter input
//--------------------------------------------------------------------------------------------------

BEGIN_SHADER_PARAMETER_STRUCT(FSNNInput, OSERENDERER_API)
   SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Viewport)
   SHADER_PARAMETER(FScreenTransform, ToOutput)
   SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Texture)
   SHADER_PARAMETER_SAMPLER(SamplerState, Sampler)
END_SHADER_PARAMETER_STRUCT()


//--------------------------------------------------------------------------------------------------
/// SNN shader definition
//--------------------------------------------------------------------------------------------------

class OSERENDERER_API FSNNShaderPS : public FGlobalShader
{
   DECLARE_GLOBAL_SHADER(FSNNShaderPS);
   SHADER_USE_PARAMETER_STRUCT(FSNNShaderPS, FGlobalShader);

   class FPassNearest : SHADER_PERMUTATION_BOOL("SNN_ENABLE_PASS_NEAREST");
   class FPassDistant : SHADER_PERMUTATION_BOOL("SNN_ENABLE_PASS_DISTANT");
   using FPermutationDomain = TShaderPermutationDomain<FPassNearest, FPassDistant>;

   static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& inParameters, FShaderCompilerEnvironment& outEnvironment)
   {
      FGlobalShader::ModifyCompilationEnvironment(inParameters, outEnvironment);
   }

   static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& inParameters)
   {
      return IsFeatureLevelSupported(inParameters.Platform, ERHIFeatureLevel::SM5);
   }

   BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
      SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
      SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
      SHADER_PARAMETER_STRUCT(FSNNInput, Input0)
      SHADER_PARAMETER_STRUCT(FSNNInput, Input1)
      SHADER_PARAMETER_STRUCT_REF(FBlueNoise, BlueNoise)
      RENDER_TARGET_BINDING_SLOTS()
   END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FSNNShaderPS, "/Plugin/OSECore/SNN.usf", "FinalPS", SF_Pixel);


//--------------------------------------------------------------------------------------------------
/// SNN Scene View Extension
//--------------------------------------------------------------------------------------------------

FOSEScreenPassSNN::FOSEScreenPassSNN(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem)
   : FOSESceneViewExtension(inAutoRegister, inRenderSubsystem)
{
   // This effect is only allowed to render on the active, primary viewport
   IsActiveThisFrameFunctions.Add(FSceneViewExtensionIsActiveFunctor());
   IsActiveThisFrameFunctions.Last().IsActiveFunction =
      [](const ISceneViewExtension* extension, const FSceneViewExtensionContext& context) -> TOptional<bool>
   {
      // Check for the requisite show flags
      if (const FViewport* currentVP = context.Viewport)
      {
         if (FViewportClient* vpClient = currentVP->GetClient())
         {
            if (const FEngineShowFlags* showFlags = vpClient->GetEngineShowFlags())
            {
               if (!SNN::ShowFlag.IsEnabled(*showFlags))
               {
                  return TOptional<bool>(false);
               }
            }
         }
      }

      // Defer to subsequent functor(s)
      return TOptional<bool>();
   };

   // Add per-frame 'is active' functor
   IsActiveThisFrameFunctions.Add(FSceneViewExtensionIsActiveFunctor());
   IsActiveThisFrameFunctions.Last().IsActiveFunction =
      [](const ISceneViewExtension* extension, const FSceneViewExtensionContext& context) -> TOptional<bool>
   {
      // Returning false disables the extension for the current frame. This will be queried each frame to determine if the extension wants to run.
      const bool isEnabled = SNN::CVar::Enabled.GetValueOnAnyThread() != 0;
      return isEnabled;
   };
}

// Called on game thread when view family is about to be rendered
void FOSEScreenPassSNN::BeginRenderViewFamily(FSceneViewFamily& inViewFamily)
{

}

// Called on render thread at the start of rendering
void FOSEScreenPassSNN::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& inRHICmdList, FSceneViewFamily& inViewFamily)
{

}

void FOSEScreenPassSNN::SubscribeToPostProcessingPass(EPostProcessingPass inPass, FAfterPassCallbackDelegateArray& inOutPassCallbacks, bool inIsPassEnabled)
{

}

namespace SNN
{
   static FSNNInput GetInputParams(const FScreenPassTexture& inputTex, const FScreenPassTexture& outputTex)
   {
      const FScreenPassTextureViewportParameters inputViewportParams = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(inputTex));
      const FScreenPassTextureViewportParameters outputViewportParams = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(outputTex));
      using ETextureBasis = FScreenTransform::ETextureBasis;

      FSNNInput params;
      params.Viewport = inputViewportParams;
      // inlined FScreenTransform::ChangeTextureUVCoordinateFromTo
      params.ToOutput = FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(inputTex), ETextureBasis::TextureUV, ETextureBasis::ViewportUV) *
                        FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(outputTex), ETextureBasis::ViewportUV, ETextureBasis::TextureUV);
      params.Texture = inputTex.Texture;
      params.Sampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
      return params;
   }

   // Internal wrapper method to render a pass w/ varying params
   static void RenderPass(
      FRDGBuilder& inGraphBuilder,
      FRDGEventName&& passName,
      const FViewInfo& viewInfo,
      const FPostProcessingInputs& inPostProcInputs,
      const FSNNShaderPS::FPermutationDomain& inPermutation,
      const FScreenPassRenderTarget& inputTex,
      const FScreenPassTexture& inputDepth,
      const FScreenPassRenderTarget& outputTex)
   {
      // Viewport remapping
      const FScreenPassTextureViewport inputViewport(inputTex);
      const FScreenPassTextureViewport outputViewport(outputTex);

      FBlueNoise blueNoise = GetBlueNoiseGlobalParameters();

      // Parameters
      auto* passParameters = inGraphBuilder.AllocParameters<FSNNShaderPS::FParameters>();
      passParameters->View = viewInfo.ViewUniformBuffer;
      passParameters->SceneTextures = inPostProcInputs.SceneTextures;
      passParameters->Input0 = SNN::GetInputParams(inputTex, outputTex);
      passParameters->Input1 = SNN::GetInputParams(inputDepth, outputTex);
      passParameters->BlueNoise = CreateUniformBufferImmediate(blueNoise, EUniformBufferUsage::UniformBuffer_SingleDraw);
      passParameters->RenderTargets[0] = outputTex.GetRenderTargetBinding();

      TShaderMapRef<FSNNShaderPS> pixelShader(viewInfo.ShaderMap, inPermutation);
      AddDrawScreenPass(inGraphBuilder, Forward<FRDGEventName&&>(passName), viewInfo, outputViewport, inputViewport, pixelShader, passParameters);
   }
}


// Called right before Post Processing rendering begins
void FOSEScreenPassSNN::PrePostProcessPass_RenderThread(FRDGBuilder& inGraphBuilder, const FSceneView& inView, const FPostProcessingInputs& inPostProcInputs)
{
   check(IsInRenderingThread());
   check(inView.VerifyMembersChecks());
   inPostProcInputs.Validate();

   // FViewInfo is always passed to this callback (as FSceneView)
   const FViewInfo& viewInfo = static_cast<const FViewInfo&>(inView);
   const FIntRect primaryViewRect = viewInfo.ViewRect;

   const FScreenPassRenderTarget sceneColor((*inPostProcInputs.SceneTextures)->SceneColorTexture, primaryViewRect, ERenderTargetLoadAction::ENoAction);
   const FScreenPassTexture sceneDepth((*inPostProcInputs.SceneTextures)->SceneDepthTexture, primaryViewRect);

   // Allocate an intermediate output based off of input texture
   const FScreenPassRenderTarget outputRT = FScreenPassRenderTarget::CreateFromInput(
      inGraphBuilder, sceneColor, ERenderTargetLoadAction::ENoAction, TEXT("OSE.SNN.Intermediate"));

   // @TODO: This could be used if we needed to deal with the final view output (either directly or just to match the format),
   // or if we were skipping post processing.
   // 
   //    FScreenPassRenderTarget outputRT = FScreenPassRenderTarget::CreateViewFamilyOutput(inPostProcInputs.ViewFamilyTexture, viewInfo);

   RDG_EVENT_SCOPE(inGraphBuilder, "OSE.SNN");
   {
      // Pass 0
      {
         FSNNShaderPS::FPermutationDomain permutationVector;
         permutationVector.Set<FSNNShaderPS::FPassDistant>(true);
         permutationVector.Set<FSNNShaderPS::FPassNearest>(true);

         SNN::RenderPass(
            inGraphBuilder, RDG_EVENT_NAME("SNN_Distant"),
            viewInfo, inPostProcInputs,
            permutationVector,
            sceneColor, sceneDepth,
            outputRT);
      }

      // Pass 1
      {
         FSNNShaderPS::FPermutationDomain permutationVector;
         permutationVector.Set<FSNNShaderPS::FPassDistant>(false);
         permutationVector.Set<FSNNShaderPS::FPassNearest>(true);

         SNN::RenderPass(
            inGraphBuilder, RDG_EVENT_NAME("SNN_Nearest"),
            viewInfo, inPostProcInputs,
            permutationVector,
            outputRT, sceneDepth,
            sceneColor);
      }
   }
}
