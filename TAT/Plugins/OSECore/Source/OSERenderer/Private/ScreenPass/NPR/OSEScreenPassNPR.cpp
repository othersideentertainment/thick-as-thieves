// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "ScreenPass/NPR/OSEScreenPassNPR.h"
#include "OSERenderSubsystem.h"

// UE4
#include "Runtime/RenderCore/Public/RenderGraphUtils.h"
#include "Runtime/Renderer/Private/PostProcess/PostProcessing.h"
#include "Runtime/Renderer/Private/FogRendering.h"
#include "DataDrivenShaderPlatformInfo.h"


//--------------------------------------------------------------------------------------------------
/// NPR console variables
//--------------------------------------------------------------------------------------------------

namespace NPR
{
   namespace CVar
   {
      TAutoConsoleVariable<int32> Enabled(
         TEXT("OSE.NPR.Enabled"),
         0,
         TEXT("Enables or disables the NPR post process\n")
         TEXT(" 0: disabled (default)\n")
         TEXT(" 1: enabled"),
         ECVF_RenderThreadSafe);

      namespace Outline
      {
         namespace Depth
         {
            TAutoConsoleVariable<float> Amt(
               TEXT("OSE.NPR.Outline.Depth.Amt"),
               1.0f,
               TEXT("The amount (0..1) to weight this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Min(
               TEXT("OSE.NPR.Outline.Depth.Min"),
               20.0f,
               TEXT("The minimum value when comparing this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Max(
               TEXT("OSE.NPR.Outline.Depth.Max"),
               50.0f,
               TEXT("The maximum value when comparing this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Exp(
               TEXT("OSE.NPR.Outline.Depth.Exp"),
               1.0f,
               TEXT("The power to raise this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Dst(
               TEXT("OSE.NPR.Outline.Depth.Dst"),
               4.0f,
               TEXT("The distance factor to apply to this attribute"),
               ECVF_RenderThreadSafe);
         }

         namespace ViewNormal
         {
            TAutoConsoleVariable<float> Amt(
               TEXT("OSE.NPR.Outline.ViewNormal.Amt"),
               1.0f,
               TEXT("The amount (0..1) to weight this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Min(
               TEXT("OSE.NPR.Outline.ViewNormal.Min"),
               0.1f,
               TEXT("The minimum value when comparing this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Max(
               TEXT("OSE.NPR.Outline.ViewNormal.Max"),
               0.3f,
               TEXT("The maximum value when comparing this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Exp(
               TEXT("OSE.NPR.Outline.ViewNormal.Exp"),
               5.0f,
               TEXT("The power to raise this attribute"),
               ECVF_RenderThreadSafe);

            TAutoConsoleVariable<float> Dst(
               TEXT("OSE.NPR.Outline.ViewNormal.Dst"),
               2.0f,
               TEXT("The distance factor to apply to this attribute"),
               ECVF_RenderThreadSafe);
         }
      }
   }
}


//--------------------------------------------------------------------------------------------------
/// NPR outline parameters input
//--------------------------------------------------------------------------------------------------

BEGIN_SHADER_PARAMETER_STRUCT(FNPROutlineAttrib, OSERENDERER_API)
   SHADER_PARAMETER(float, Amt)
   SHADER_PARAMETER(float, Min)
   SHADER_PARAMETER(float, Max)
   SHADER_PARAMETER(float, Exp)
   SHADER_PARAMETER(float, Dst)
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FNPROutlineParams, OSERENDERER_API)
   SHADER_PARAMETER_STRUCT(FNPROutlineAttrib, Depth)
   SHADER_PARAMETER_STRUCT(FNPROutlineAttrib, ViewNormal)
END_SHADER_PARAMETER_STRUCT()


//--------------------------------------------------------------------------------------------------
/// NPR texture parameter input
//--------------------------------------------------------------------------------------------------

BEGIN_SHADER_PARAMETER_STRUCT(FNPRInput, OSERENDERER_API)
   SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Viewport)
   SHADER_PARAMETER(FScreenTransform, ToOutput)
   SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Texture)
   SHADER_PARAMETER_SAMPLER(SamplerState, Sampler)
END_SHADER_PARAMETER_STRUCT()


//--------------------------------------------------------------------------------------------------
/// NPR shader definition
//--------------------------------------------------------------------------------------------------

class OSERENDERER_API FNPRShaderPS : public FGlobalShader
{
   DECLARE_GLOBAL_SHADER(FNPRShaderPS);
   SHADER_USE_PARAMETER_STRUCT(FNPRShaderPS, FGlobalShader);

   class FEnableOutline               : SHADER_PERMUTATION_BOOL("NPR_ENABLE_OUTLINE");
   class FEnableOutlineDepth          : SHADER_PERMUTATION_BOOL("NPR_ENABLE_OUTLINE_DEPTH");
   class FEnableOutlineDepthDist      : SHADER_PERMUTATION_BOOL("NPR_ENABLE_OUTLINE_DEPTH_WORLDDIST");
   class FEnableOutlineViewNormal     : SHADER_PERMUTATION_BOOL("NPR_ENABLE_OUTLINE_VIEWNORMAL");
   class FEnableOutlineViewNormalDist : SHADER_PERMUTATION_BOOL("NPR_ENABLE_OUTLINE_VIEWNORMAL_WORLDDIST");
   class FEnableLumaDec               : SHADER_PERMUTATION_BOOL("NPR_ENABLE_LUMA_DEC");
   using FPermutationDomain = TShaderPermutationDomain
      < FEnableOutline
      , FEnableOutlineDepth
      , FEnableOutlineDepthDist
      , FEnableOutlineViewNormal
      , FEnableOutlineViewNormalDist
      , FEnableLumaDec >;

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
      SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFogUniformParameters, FogStruct)
      SHADER_PARAMETER_STRUCT(FNPRInput, Input0)
      SHADER_PARAMETER_STRUCT(FNPROutlineParams, Outline)
      RENDER_TARGET_BINDING_SLOTS()
   END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FNPRShaderPS, "/Plugin/OSECore/NPR.usf", "FinalPS", SF_Pixel);


//--------------------------------------------------------------------------------------------------
/// NPR Scene View Extension
//--------------------------------------------------------------------------------------------------

FOSEScreenPassNPR::FOSEScreenPassNPR(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem)
   : FOSESceneViewExtension(inAutoRegister, inRenderSubsystem)
{
   // Add per-frame 'is active' functor
   IsActiveThisFrameFunctions.Add(FSceneViewExtensionIsActiveFunctor());
   IsActiveThisFrameFunctions.Last().IsActiveFunction = 
      [](const ISceneViewExtension* extension, const FSceneViewExtensionContext& context) -> TOptional<bool>
      {
         // Returning false disables the extension for the current frame. This will be queried each frame to determine if the extension wants to run.
         const bool isEnabled = NPR::CVar::Enabled.GetValueOnAnyThread() != 0;
         return isEnabled;
      };
}


namespace NPR
{
   static FNPRInput GetInputParams(const FScreenPassTexture& inputTex, const FScreenPassTexture& outputTex)
   {
      const FScreenPassTextureViewportParameters inputViewportParams = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(inputTex));
      const FScreenPassTextureViewportParameters outputViewportParams = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(outputTex));
      using ETextureBasis = FScreenTransform::ETextureBasis;

      FNPRInput params;
      params.Viewport = inputViewportParams;
      // inlined FScreenTransform::ChangeTextureUVCoordinateFromTo
      params.ToOutput = FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(inputTex), ETextureBasis::TextureUV, ETextureBasis::ViewportUV) *
                        FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(outputTex), ETextureBasis::ViewportUV, ETextureBasis::TextureUV);
      params.Texture = inputTex.Texture;
      params.Sampler = TStaticSamplerState<>::GetRHI();
      return params;
   }

   // Internal wrapper method to render a pass w/ varying params
   static void RenderPass(
      FRDGBuilder& inGraphBuilder,
      FRDGEventName&& passName,
      const FViewInfo& viewInfo,
      TRDGUniformBufferRef<FSceneTextureUniformParameters> inSceneTextures,
      FNPRShaderPS::FPermutationDomain inPermutation,
      const FScreenPassRenderTarget& inputTex,
      const FScreenPassRenderTarget& outputTex,
      FRHIBlendState* blendState = FScreenPassPipelineState::FDefaultBlendState::GetRHI())
   {
      // Viewport remapping
      const FScreenPassTextureViewport inputViewport(inputTex);
      const FScreenPassTextureViewport outputViewport(outputTex);

      // Parameters
      auto* passParameters = inGraphBuilder.AllocParameters<FNPRShaderPS::FParameters>();
      passParameters->View = viewInfo.ViewUniformBuffer;
      passParameters->SceneTextures = inSceneTextures;
      passParameters->FogStruct = CreateFogUniformBuffer(inGraphBuilder, viewInfo);
      passParameters->Input0 = NPR::GetInputParams(inputTex, outputTex);
      passParameters->RenderTargets[0] = outputTex.GetRenderTargetBinding();

      passParameters->Outline.Depth.Amt = NPR::CVar::Outline::Depth::Amt.GetValueOnRenderThread();
      passParameters->Outline.Depth.Min = NPR::CVar::Outline::Depth::Min.GetValueOnRenderThread();
      passParameters->Outline.Depth.Max = NPR::CVar::Outline::Depth::Max.GetValueOnRenderThread();
      passParameters->Outline.Depth.Exp = NPR::CVar::Outline::Depth::Exp.GetValueOnRenderThread();
      passParameters->Outline.Depth.Dst = NPR::CVar::Outline::Depth::Dst.GetValueOnRenderThread();

      passParameters->Outline.ViewNormal.Amt = NPR::CVar::Outline::ViewNormal::Amt.GetValueOnRenderThread();
      passParameters->Outline.ViewNormal.Min = NPR::CVar::Outline::ViewNormal::Min.GetValueOnRenderThread();
      passParameters->Outline.ViewNormal.Max = NPR::CVar::Outline::ViewNormal::Max.GetValueOnRenderThread();
      passParameters->Outline.ViewNormal.Exp = NPR::CVar::Outline::ViewNormal::Exp.GetValueOnRenderThread();
      passParameters->Outline.ViewNormal.Dst = NPR::CVar::Outline::ViewNormal::Dst.GetValueOnRenderThread();

      // Depth specific permutations
      {
         const bool hasAmount   = (passParameters->Outline.Depth.Amt > 0);
         const bool hasDistance = (passParameters->Outline.Depth.Dst > 0);
         inPermutation.Set<FNPRShaderPS::FEnableOutlineDepth>(hasAmount);
         inPermutation.Set<FNPRShaderPS::FEnableOutlineDepthDist>(hasAmount && hasDistance);
      }

      // ViewNormal specific permutations
      {
         const bool hasAmount   = (passParameters->Outline.ViewNormal.Amt > 0);
         const bool hasDistance = (passParameters->Outline.ViewNormal.Dst > 0);
         inPermutation.Set<FNPRShaderPS::FEnableOutlineViewNormal>(hasAmount);
         inPermutation.Set<FNPRShaderPS::FEnableOutlineViewNormalDist>(hasAmount && hasDistance);
      }

      TShaderMapRef<FScreenPassVS> vertexShader(viewInfo.ShaderMap);
      TShaderMapRef<FNPRShaderPS> pixelShader(viewInfo.ShaderMap, inPermutation);
      AddDrawScreenPass(inGraphBuilder, Forward<FRDGEventName&&>(passName), viewInfo,
         outputViewport, inputViewport,
         vertexShader, pixelShader, blendState, passParameters);
   }

   static void RenderAllPasses(
      FRDGBuilder& inGraphBuilder,
      const FViewInfo& viewInfo,
      TRDGUniformBufferRef<FSceneTextureUniformParameters> inSceneTextures
   )
   {
      const FIntRect primaryViewRect = viewInfo.ViewRect;

      const FScreenPassRenderTarget sceneColor((*inSceneTextures)->SceneColorTexture, primaryViewRect, ERenderTargetLoadAction::ENoAction);
   
      // Allocate an intermediate output based off of input texture
      const FScreenPassRenderTarget outputRT = FScreenPassRenderTarget::CreateFromInput(
         inGraphBuilder, sceneColor, ERenderTargetLoadAction::ENoAction, TEXT("OSE.NPR.Intermediate"));

      RDG_EVENT_SCOPE(inGraphBuilder, "OSE.NPR");
      {
         // Pass 0
         {
            FNPRShaderPS::FPermutationDomain permutationVector;
            permutationVector.Set<FNPRShaderPS::FEnableOutline>(true);
            permutationVector.Set<FNPRShaderPS::FEnableLumaDec>(false);

            NPR::RenderPass(
               inGraphBuilder, RDG_EVENT_NAME("NPR_0"),
               viewInfo, inSceneTextures,
               permutationVector,
               sceneColor,
               outputRT);
         }

         // Pass 1
         {
            FNPRShaderPS::FPermutationDomain permutationVector;
            permutationVector.Set<FNPRShaderPS::FEnableOutline>(false);
            permutationVector.Set<FNPRShaderPS::FEnableLumaDec>(true);

            NPR::RenderPass(
               inGraphBuilder, RDG_EVENT_NAME("NPR_1"),
               viewInfo, inSceneTextures,
               permutationVector,
               outputRT,
               sceneColor);
         }
      }
   }
}

void FOSEScreenPassNPR::PostRenderLightingDeferred_RenderThread(FRDGBuilder& inGraphBuilder, FSceneView& inView, TRDGUniformBufferRef<FSceneTextureUniformParameters> inSceneTextures)
{
   check(IsInRenderingThread());
   check(inView.VerifyMembersChecks());

   // FViewInfo is always passed to this callback (as FSceneView)
   const FViewInfo& viewInfo = static_cast<const FViewInfo&>(inView);

   NPR::RenderAllPasses(inGraphBuilder, viewInfo, inSceneTextures);
}

void FOSEScreenPassNPR::PrePostProcessPass_RenderThread(FRDGBuilder& inGraphBuilder, const FSceneView& inView, const FPostProcessingInputs& inPostProcInputs)
{
   check(IsInRenderingThread());
   check(inView.VerifyMembersChecks());
   inPostProcInputs.Validate();

   // @TODO: Perform full-scene level effects here, such as luma decimation, etc.
   /*
   // FViewInfo is always passed to this callback (as FSceneView)
   const FViewInfo& viewInfo = static_cast<const FViewInfo&>(inView);
   NPR::RenderAllPasses(inGraphBuilder, viewInfo, inPostProcInputs.SceneTextures);
   */
}
