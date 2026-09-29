// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <SceneViewExtension.h>

class UOSEXrayComponent;

struct FOSEXrayPrimitive
{
   FPrimitiveComponentId ComponentId;
   float MaxDrawDistanceSq = 0.0f;

   FMaterialRenderProxy* MaterialRenderProxy = nullptr;

   bool bSkipHidden = true;
   bool bSkipOccluded = false;
   bool bSkipNonOccluded = false;
   bool bDepthOnly = false;
};

class FOSEXraySceneViewExtension : public FSceneViewExtensionBase
{
public:
   FOSEXraySceneViewExtension(const FAutoRegister& autoRegister);

   virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& context) const override;
   virtual void SetupViewFamily(FSceneViewFamily& viewFamily) override;
   virtual void SetupView(FSceneViewFamily& viewFamily, FSceneView& view) override;
   virtual void BeginRenderViewFamily(FSceneViewFamily& viewFamily) override;
   virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& graphBuilder, FSceneView& view, const FRenderTargetBindingSlots& renderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> sceneTexturesUB) override;

   TArray<TObjectPtr<UOSEXrayComponent>> RegisteredComponents;
   TArray<FOSEXrayPrimitive> PrimitivesRT;
};
