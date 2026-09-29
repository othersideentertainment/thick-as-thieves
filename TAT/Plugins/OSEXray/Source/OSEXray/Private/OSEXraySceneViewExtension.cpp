// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEXraySceneViewExtension.h"

// OSE
#include "OSEXrayComponent.h"
#include "OSEXrayShaders.h"

// UE
#include <MeshPassProcessor.h>
#include <MeshPassProcessor.inl>
#include <PixelShaderUtils.h>
#include <PrimitiveSceneInfo.h>
#include <ScenePrivate.h>
#include <SceneRendering.h>
#include <SimpleMeshDrawCommandPass.h>
#include <Components/MeshComponent.h>
#include <InstanceCulling/InstanceCullingContext.h>
#include <Materials/MaterialInterface.h>

DECLARE_GPU_STAT_NAMED(OSEXray, TEXT("OSE Xray"));
DECLARE_GPU_STAT_NAMED(OSEXrayMaskPass, TEXT("OSE Xray Mask Pass"));
DECLARE_GPU_STAT_NAMED(OSEXrayDepthPass, TEXT("OSE Xray Depth Pass"));
DECLARE_GPU_STAT_NAMED(OSEXrayMeshPass, TEXT("OSE Xray Mesh Pass"));

namespace OSE::Xray
{
   static bool bEnabled = true;
   FAutoConsoleVariableRef CVarEnabled(TEXT("OSE.Xray.Enabled"), bEnabled, TEXT(""));
}

BEGIN_SHADER_PARAMETER_STRUCT(FOSEXrayDepthPassParameters,)
   SHADER_PARAMETER_STRUCT_INCLUDE(FViewShaderParameters, View)
   SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters, Scene)
   SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
   RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

class FOSEXrayDepthPassProcessor : public FMeshPassProcessor
{
public:
   FOSEXrayDepthPassProcessor(const FScene* scene, const FSceneView* view, FMeshPassDrawListContext* drawListContext)
      : FMeshPassProcessor(TEXT("OSEXrayDepthPass"), scene, view->GetFeatureLevel(), view, drawListContext)
   {
      PassDrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());
      PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<
         true, CF_DepthNearOrEqual,
         true, CF_Equal, SO_Keep, SO_Keep, SO_Keep,
         false, CF_Always, SO_Keep, SO_Keep, SO_Keep,
         0x1, 0x0
      >::GetRHI());
   }

   static bool GetDepthPassMaterialShaders(const FMaterial& material, const FVertexFactoryType* vertexFactoryType, bool bMaterialUsesPixelDepthOffset, bool bPositionOnly, FMaterialShaders& shaders)
   {
      // Based on GetDepthPassShaders; gather shaders using reflection as they're not exposed outside the render module
      static const FShaderPipelineType* DepthPipeline = FShaderPipelineType::GetShaderPipelineTypeByName(TEXT("DepthPipeline"));
      static const FShaderPipelineType* DepthNoPixelPipeline = FShaderPipelineType::GetShaderPipelineTypeByName(TEXT("DepthNoPixelPipeline"));
      static const FShaderPipelineType* DepthPosOnlyNoPixelPipeline = FShaderPipelineType::GetShaderPipelineTypeByName(TEXT("DepthPosOnlyNoPixelPipeline"));

      FMaterialShaderTypes shaderTypes;
      if (bPositionOnly)
      {
         shaderTypes.PipelineType = DepthPosOnlyNoPixelPipeline;
         shaderTypes.AddShaderType(shaderTypes.PipelineType->GetShader(SF_Vertex));
      }
      else
      {
         const bool bSupportsNullPixelShader = vertexFactoryType->SupportsNullPixelShader();
         const bool bNeedsPixelShader = !material.WritesEveryPixel(false, bSupportsNullPixelShader) || bMaterialUsesPixelDepthOffset || material.IsTranslucencyWritingCustomDepth();

         if (bNeedsPixelShader)
         {
            shaderTypes.PipelineType = DepthPipeline;
            shaderTypes.AddShaderType(shaderTypes.PipelineType->GetShader(SF_Vertex));
            shaderTypes.AddShaderType(shaderTypes.PipelineType->GetShader(SF_Pixel));
         }
         else
         {
            shaderTypes.PipelineType = DepthNoPixelPipeline;
            shaderTypes.AddShaderType(shaderTypes.PipelineType->GetShader(SF_Vertex));
         }
      }

      return material.TryGetShaders(shaderTypes, vertexFactoryType, shaders);
   }

   virtual void AddMeshBatch(const FMeshBatch& RESTRICT meshBatch, uint64 batchElementMask, const FPrimitiveSceneProxy* RESTRICT primitiveSceneProxy, int32 staticMeshId = -1) override
   {
      const FMaterialRenderProxy* materialRenderProxy = meshBatch.MaterialRenderProxy;
      while (materialRenderProxy)
      {
         const FMaterial* material = materialRenderProxy->GetMaterialNoFallback(FeatureLevel);
         if (!material)
         {
            materialRenderProxy = materialRenderProxy->GetFallback(FeatureLevel);
            continue;
         }

         const bool bSupportsPositionOnlyStream = meshBatch.VertexFactory->SupportsPositionOnlyStream();
         const bool bSupportsNullPixelShader = meshBatch.VertexFactory->SupportsNullPixelShader();
         const bool bModifiesMeshPosition = DoMaterialAndPrimitiveModifyMeshPosition(*material, primitiveSceneProxy);
         const bool bWritesEveryPixel = material->WritesEveryPixel(false, bSupportsNullPixelShader);

         bool bPositionOnly = false;
         bool bUseDefaultMaterial = false;

         // Based on FCustomDepthPassMeshProcessor::UseDefaultMaterial
         if (IsOpaqueBlendMode(*material) && bSupportsPositionOnlyStream && !bModifiesMeshPosition && bWritesEveryPixel)
         {
            bPositionOnly = true;
            bUseDefaultMaterial = true;
         }
         else if (!IsTranslucentBlendMode(*material) || material->IsTranslucencyWritingCustomDepth())
         {
            const bool bMaterialMasked = !bWritesEveryPixel || material->IsTranslucencyWritingCustomDepth();
            if (!bMaterialMasked && !bModifiesMeshPosition)
            {
               bPositionOnly = false;
               bUseDefaultMaterial = true;
            }
         }
         else
         {
            // Skip translucent materials w/o depth write
            break;
         }

         if (bUseDefaultMaterial)
         {
            materialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
            material = materialRenderProxy->GetMaterialNoFallback(FeatureLevel);
         }

         FMaterialShaders shaders;
         if (!GetDepthPassMaterialShaders(*material, meshBatch.VertexFactory->GetType(), material->MaterialUsesPixelDepthOffset_RenderThread(), bPositionOnly, shaders))
         {
            materialRenderProxy = materialRenderProxy->GetFallback(FeatureLevel);
            continue;
         }

         if (bPositionOnly)
         {
            Process<true>(meshBatch, batchElementMask, primitiveSceneProxy, staticMeshId, *materialRenderProxy, *material, shaders);
         }
         else
         {
            Process<false>(meshBatch, batchElementMask, primitiveSceneProxy, staticMeshId, *materialRenderProxy, *material, shaders);
         }

         break;
      }
   }

   template<bool bPositionOnly>
   void Process(const FMeshBatch& RESTRICT meshBatch, uint64 batchElementMask, const FPrimitiveSceneProxy* RESTRICT primitiveSceneProxy, int32 staticMeshId,
                const FMaterialRenderProxy& materialRenderProxy, const FMaterial& material, const FMaterialShaders& shaders)
   {
      TMeshProcessorShaders<TDepthOnlyVS<bPositionOnly>, FDepthOnlyPS> passShaders;
      shaders.TryGetVertexShader(passShaders.VertexShader);
      shaders.TryGetPixelShader(passShaders.PixelShader);

      const FMeshDrawingPolicyOverrideSettings overrideSettings = ComputeMeshOverrideSettings(meshBatch);
      const ERasterizerFillMode meshFillMode = ComputeMeshFillMode(material, overrideSettings);
      const ERasterizerCullMode meshCullMode = ComputeMeshCullMode(material, overrideSettings);

      const FMeshDrawCommandSortKey sortKey = CalculateMeshStaticSortKey(passShaders.VertexShader, passShaders.PixelShader);

      FMeshMaterialShaderElementData shaderElementData;
      shaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, primitiveSceneProxy, meshBatch, staticMeshId, false);

      BuildMeshDrawCommands(
         meshBatch, batchElementMask, primitiveSceneProxy,
         materialRenderProxy, material, PassDrawRenderState,
         passShaders, meshFillMode, meshCullMode, sortKey,
         bPositionOnly ? EMeshPassFeatures::PositionOnly : EMeshPassFeatures::Default,
         shaderElementData);
   }

   FMeshPassProcessorRenderState PassDrawRenderState;
};

BEGIN_SHADER_PARAMETER_STRUCT(FOSEXrayMeshPassParameters,)
   SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
   SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters, Scene)
   SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
   RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

class FOSEXrayMeshPassProcessor : public FMeshPassProcessor
{
public:
   FOSEXrayMeshPassProcessor(const FScene* scene, const FSceneView* view, FMeshPassDrawListContext* drawListContext)
      : FMeshPassProcessor(TEXT("OSEXrayMeshPass"), scene, view->GetFeatureLevel(), view, drawListContext)
   {
      PassDrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());
      PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<
         false, CF_DepthNearOrEqual,
         true, CF_Equal, SO_Keep, SO_Keep, SO_Keep,
         false, CF_Always, SO_Keep, SO_Keep, SO_Keep,
         0x1, 0x0
      >::GetRHI());
   }

   virtual void AddMeshBatch(const FMeshBatch& RESTRICT meshBatch, uint64 batchElementMask, const FPrimitiveSceneProxy* RESTRICT primitiveSceneProxy, int32 staticMeshId = -1) override
   {
      const FMaterialRenderProxy* materialRenderProxy = meshBatch.MaterialRenderProxy;
      while (materialRenderProxy)
      {
         const FMaterial* material = materialRenderProxy->GetMaterialNoFallback(FeatureLevel);
         if (!material)
         {
            materialRenderProxy = materialRenderProxy->GetFallback(FeatureLevel);
            continue;
         }

         FMaterialShaderTypes shaderTypes;
         shaderTypes.AddShaderType<FOSEXrayMeshVS>();
         shaderTypes.AddShaderType<FOSEXrayMeshPS>();

         FMaterialShaders shaders;
         if (!material->TryGetShaders(shaderTypes, meshBatch.VertexFactory->GetType(), shaders))
         {
            materialRenderProxy = materialRenderProxy->GetFallback(FeatureLevel);
            continue;
         }

         TMeshProcessorShaders<FOSEXrayMeshVS, FOSEXrayMeshPS> passShaders;
         shaders.TryGetVertexShader(passShaders.VertexShader);
         shaders.TryGetPixelShader(passShaders.PixelShader);

         const FMeshDrawingPolicyOverrideSettings overrideSettings = ComputeMeshOverrideSettings(meshBatch);
         const ERasterizerFillMode meshFillMode = ComputeMeshFillMode(*material, overrideSettings);
         const ERasterizerCullMode meshCullMode = ComputeMeshCullMode(*material, overrideSettings);

         const FMeshDrawCommandSortKey sortKey = CalculateMeshStaticSortKey(passShaders.VertexShader, passShaders.PixelShader);

         FMeshMaterialShaderElementData shaderElementData;
         shaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, primitiveSceneProxy, meshBatch, staticMeshId, true);

         BuildMeshDrawCommands(
            meshBatch, batchElementMask, primitiveSceneProxy,
            *materialRenderProxy, *material, PassDrawRenderState,
            passShaders, meshFillMode, meshCullMode, sortKey,
            EMeshPassFeatures::Default, shaderElementData
         );

         break;
      }
   }

   FMeshPassProcessorRenderState PassDrawRenderState;
};

FOSEXraySceneViewExtension::FOSEXraySceneViewExtension(const FAutoRegister& autoRegister)
   : FSceneViewExtensionBase(autoRegister)
{
}

bool FOSEXraySceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& context) const
{
   return OSE::Xray::bEnabled;
}

void FOSEXraySceneViewExtension::SetupViewFamily(FSceneViewFamily& viewFamily)
{
}

void FOSEXraySceneViewExtension::SetupView(FSceneViewFamily& viewFamily, FSceneView& view)
{
}

void FOSEXraySceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& viewFamily)
{
   TArray<FOSEXrayPrimitive> primitives;
   for (const UOSEXrayComponent* component : RegisteredComponents)
   {
      if (!component->IsEnabled())
      {
         continue;
      }

      const UMaterialInterface* material = component->GetMaterial();
      FMaterialRenderProxy* materialRenderProxy = material ? material->GetRenderProxy() : nullptr;

      const float maxDrawDistanceSq = FMath::Square(component->GetMaxDrawDistance());
      bool bSkipHiddenPrimitives = !component->ShouldRenderHiddenPrimitives();
      bool bSkipOccludedPrimitives = !component->ShouldRenderOccludedPrimitives();
      bool bSkipNonOccludedPrimitives = !component->ShouldRenderNonOccludedPrimitives();

      const AActor* owner = component->GetOwner();
      TInlineComponentArray<UMeshComponent*> meshComponents(owner);

      primitives.Reserve(primitives.Num() + meshComponents.Num());

      for (const UMeshComponent* meshComponent : meshComponents)
      {
         static const FName kXrayExclude = TEXT("XrayExclude");
         static const FName kXrayDepthOnly = TEXT("XrayDepthOnly");

         if (meshComponent->ComponentHasTag(kXrayExclude))
         {
            continue;
         }

         FOSEXrayPrimitive& primitive = primitives.Emplace_GetRef();

         primitive.ComponentId = meshComponent->GetPrimitiveSceneId();
         primitive.MaterialRenderProxy = materialRenderProxy;
         primitive.MaxDrawDistanceSq = maxDrawDistanceSq;
         primitive.bSkipHidden = bSkipHiddenPrimitives;
         primitive.bSkipOccluded = bSkipOccludedPrimitives;
         primitive.bSkipNonOccluded = bSkipNonOccludedPrimitives;
         primitive.bDepthOnly = meshComponent->ComponentHasTag(kXrayDepthOnly);
      }
   }

   // #TODO: Probably worth skipping if primitive batches don't change which is likely the common case

   ENQUEUE_RENDER_COMMAND(OSEXrayCopyPrimitives)(
      [weakThis = AsWeak(), primitives = MoveTemp(primitives)](FRHICommandListImmediate&)
      {
         const TSharedPtr<FOSEXraySceneViewExtension> sharedThis = StaticCastSharedPtr<FOSEXraySceneViewExtension>(weakThis.Pin());
         if (sharedThis.IsValid())
         {
            sharedThis->PrimitivesRT = primitives;
         }
      });
}

void FOSEXraySceneViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& graphBuilder, FSceneView& view, const FRenderTargetBindingSlots& renderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> sceneTexturesUB)
{
   RDG_EVENT_SCOPE_STAT(graphBuilder, OSEXray, "OSE Xray");
   RDG_GPU_STAT_SCOPE(graphBuilder, OSEXray);

   const FViewInfo& viewInfo = static_cast<FViewInfo&>(view);

   const FScene* scene = viewInfo.Family->Scene->GetRenderScene();
   const FSceneTextures& sceneTextures = viewInfo.GetSceneTextures();

   const FVector viewOrigin = viewInfo.ViewMatrices.GetViewOrigin();
   const double currentWorldTime = viewInfo.Family->Time.GetWorldTimeSeconds();

   struct FOSEXrayMeshBatch : FMeshBatch
   {
      FPrimitiveSceneProxy* PrimitiveSceneProxy;
      bool bDepthOnly;
   };

   TArray<FOSEXrayMeshBatch> meshBatches;
   meshBatches.Reserve(PrimitivesRT.Num());

   for (const FOSEXrayPrimitive& primitive : PrimitivesRT)
   {
      // #TODO: This lookup is the most expensive part; look into ISceneExtension to reactively gather primitives as
      // they are added/removed from the scene which would allow using the packed index for scene array lookups directly,
      // though would require refactoring how we gather info from Xray components - likely using render dynamic data?
      const int32 primitiveIdx = scene->PrimitiveComponentIds.Find(primitive.ComponentId);
      if (!scene->Primitives.IsValidIndex(primitiveIdx))
      {
         continue;
      }

      FPrimitiveSceneInfo* primitiveSceneInfo = scene->Primitives[primitiveIdx];
      check(primitiveSceneInfo);

      FPrimitiveSceneProxy* primitiveSceneProxy = primitiveSceneInfo->Proxy;
      check(primitiveSceneProxy);

      const bool bIsHidden = !primitiveSceneProxy->ShouldRenderInMainPass() || !primitiveSceneProxy->IsShown(&viewInfo);
      if (primitive.bSkipHidden && bIsHidden)
      {
         continue;
      }

      const bool bIsOccluded = !viewInfo.PrimitiveDefinitelyUnoccludedMap[primitiveIdx];
      if (primitive.bSkipOccluded && bIsOccluded)
      {
         continue;
      }

      if (primitive.bSkipNonOccluded && !bIsOccluded)
      {
         continue;
      }

      const FPrimitiveBounds& primitiveBounds = scene->PrimitiveBounds[primitiveIdx];
      if (!viewInfo.ViewFrustum.IntersectBox(primitiveBounds.BoxSphereBounds.Origin, primitiveBounds.BoxSphereBounds.BoxExtent))
      {
         // Skip primitives that are frustum culled
         continue;
      }

      if (primitive.MaxDrawDistanceSq > 0)
      {
         // #TODO: See GDistanceCullToSphereEdge, may be more appropriate to base off bounds edge rather than origin
         const float distSq = FVector::DistSquared(primitiveBounds.BoxSphereBounds.Origin, viewOrigin);
         if (distSq > primitive.MaxDrawDistanceSq)
         {
            // Skip primitives that are distance culled
            continue;
         }
      }

      // Update component render time for dependent systems like animation
      primitiveSceneInfo->UpdateComponentLastRenderTime(currentWorldTime, true);

      // Can't rely on View.PrimitivesLODMask as it's only set for visible primitives
      const FDesiredLODLevel desiredLodLevel = primitiveSceneProxy->GetDesiredLODLevel_RenderThread(&viewInfo);

      FLODMask lodMask;
      if (desiredLodLevel.IsFixed())
      {
         lodMask.SetLOD(desiredLodLevel.LOD);
      }
      else
      {
         // #TODO: Use appropriate LODs; see ComputeLODForMeshes / CalcAndUpdateLODToRender
         int8 minLod, maxLod;
         primitiveSceneInfo->GetStaticMeshesLODRange(minLod, maxLod);

         lodMask.SetLOD(minLod);
      }

      for (int32 meshIdx = 0; meshIdx < primitiveSceneInfo->StaticMeshes.Num(); ++meshIdx)
      {
         const FStaticMeshBatch& staticMesh = primitiveSceneInfo->StaticMeshes[meshIdx];
         if (!staticMesh.bUseForMaterial || !lodMask.ContainsLOD(staticMesh.LODIndex))
         {
            continue;
         }

         check(primitiveSceneProxy == staticMesh.PrimitiveSceneInfo->Proxy);

         FOSEXrayMeshBatch& meshBatch = meshBatches.Emplace_GetRef(staticMesh);
         meshBatch.PrimitiveSceneProxy = primitiveSceneProxy;
         meshBatch.bDepthOnly = primitive.bDepthOnly;
         meshBatch.CastShadow = false;

         if (primitive.MaterialRenderProxy)
         {
            meshBatch.MaterialRenderProxy = primitive.MaterialRenderProxy;
         }
      }
   }

   FRDGTextureRef depthStencilTexture = graphBuilder.CreateTexture(sceneTextures.Depth.Target->Desc, TEXT("OSEXrayDepthStencil"));

   {
      RDG_EVENT_SCOPE_STAT(graphBuilder, OSEXrayMaskPass, "OSE Xray Mask Pass");
      RDG_GPU_STAT_SCOPE(graphBuilder, OSEXrayMaskPass);

      TShaderMapRef<FOSEXrayMaskPS> pixelShader(viewInfo.ShaderMap);

      FOSEXrayMaskPS::FParameters* passParams = graphBuilder.AllocParameters<FOSEXrayMaskPS::FParameters>();
      passParams->View = view.ViewUniformBuffer;

      // Scene textures UB is not set up with GBuffer until after base pass, gather from scene context
      // passParams->SceneTextures = GetSceneTextureParameters(graphBuilder, sceneTextures);
      passParams->SceneTextures.GBufferATexture = sceneTextures.GBufferA;
      passParams->SceneTextures.GBufferBTexture = sceneTextures.GBufferB;
      passParams->SceneTextures.GBufferCTexture = sceneTextures.GBufferC;
      passParams->SceneTextures.GBufferDTexture = sceneTextures.GBufferD;
      passParams->SceneTextures.GBufferETexture = sceneTextures.GBufferE;
      passParams->SceneTextures.GBufferVelocityTexture = sceneTextures.Velocity;

      passParams->RenderTargets.DepthStencil = FDepthStencilBinding(depthStencilTexture, ERenderTargetLoadAction::ENoAction, ERenderTargetLoadAction::EClear, FExclusiveDepthStencil::DepthNop_StencilWrite);

      FRHIDepthStencilState* depthStencilState = TStaticDepthStencilState<
         false, CF_Always,
         true, CF_Always, SO_Keep, SO_Keep, SO_Replace,
         false, CF_Always, SO_Keep, SO_Keep, SO_Keep,
         0x0, 0x1
      >::GetRHI();

      FPixelShaderUtils::AddFullscreenPass(
         graphBuilder, viewInfo.ShaderMap, RDG_EVENT_NAME("OSE Xray Mask Pass"),
         pixelShader, passParams, viewInfo.ViewRect, nullptr, nullptr, depthStencilState, 1
      );
   }

   {
      RDG_EVENT_SCOPE_STAT(graphBuilder, OSEXrayDepthPass, "OSE Xray Depth Pass");
      RDG_GPU_STAT_SCOPE(graphBuilder, OSEXrayDepthPass);

      FOSEXrayDepthPassParameters* passParams = graphBuilder.AllocParameters<FOSEXrayDepthPassParameters>();
      passParams->Scene = GetSceneUniformBufferRef(graphBuilder, viewInfo);
      passParams->View.View = viewInfo.ViewUniformBuffer;
      passParams->View.InstancedView = viewInfo.GetInstancedViewUniformBuffer();
      passParams->RenderTargets.DepthStencil = FDepthStencilBinding(depthStencilTexture, ERenderTargetLoadAction::EClear, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilRead);

      AddSimpleMeshPass(
         graphBuilder, passParams, scene, viewInfo, nullptr, RDG_EVENT_NAME("OSE Xray Depth Pass"), viewInfo.ViewRect,
         [&meshBatches, &viewInfo](FDynamicPassMeshDrawListContext* dynamicMeshPassContext)
         {
            FOSEXrayDepthPassProcessor passProcessor(nullptr, &viewInfo, dynamicMeshPassContext);

            constexpr uint64 batchElementMask = ~0ull;
            for (const FOSEXrayMeshBatch& meshBatch : meshBatches)
            {
               passProcessor.AddMeshBatch(meshBatch, batchElementMask, meshBatch.PrimitiveSceneProxy);
            }
         });
   }

   {
      RDG_EVENT_SCOPE_STAT(graphBuilder, OSEXrayMeshPass, "OSE Xray Mesh Pass");
      RDG_GPU_STAT_SCOPE(graphBuilder, OSEXrayMeshPass);

      // Manually gather render targets so we can also bind the velocity buffer
      TStaticArray<FTextureRenderTargetBinding, MaxSimultaneousRenderTargets> passRenderTargets;
      const uint32 passRenderTargetCount = sceneTextures.GetGBufferRenderTargets(passRenderTargets, GBL_ForceVelocity);

      FOSEXrayMeshPassParameters* passParams = graphBuilder.AllocParameters<FOSEXrayMeshPassParameters>();
      passParams->Scene = GetSceneUniformBufferRef(graphBuilder, viewInfo);
      passParams->View = viewInfo.ViewUniformBuffer;
      passParams->RenderTargets = GetRenderTargetBindings(ERenderTargetLoadAction::ELoad, MakeArrayView(passRenderTargets.GetData(), passRenderTargetCount));
      passParams->RenderTargets.DepthStencil = FDepthStencilBinding(depthStencilTexture, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthRead_StencilRead);

      AddSimpleMeshPass(
         graphBuilder, passParams, scene, viewInfo, nullptr, RDG_EVENT_NAME("OSE Xray Mesh Pass"), viewInfo.ViewRect,
         [&meshBatches, &viewInfo](FDynamicPassMeshDrawListContext* dynamicMeshPassContext)
         {
            FOSEXrayMeshPassProcessor passProcessor(nullptr, &viewInfo, dynamicMeshPassContext);

            constexpr uint64 batchElementMask = ~0ull;
            for (const FOSEXrayMeshBatch& meshBatch : meshBatches)
            {
               if (meshBatch.bDepthOnly)
               {
                  continue;
               }

               passProcessor.AddMeshBatch(meshBatch, batchElementMask, meshBatch.PrimitiveSceneProxy);
            }
         }
      );
   }
}
