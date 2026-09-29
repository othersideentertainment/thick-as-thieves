// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ScreenPass/OSESceneViewExtension.h"


class OSERENDERER_API FOSEScreenPassNPR : public FOSESceneViewExtension
{
public:

   FOSEScreenPassNPR(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem);

   /// Higher priority comes first; this is intended to run early
   virtual int32 GetPriority() const override { return 100; }

   /// Called right after deferred lighting is complete, before fog and translucency
   virtual void PostRenderLightingDeferred_RenderThread(FRDGBuilder& inGraphBuilder, FSceneView& inView, TRDGUniformBufferRef<FSceneTextureUniformParameters> inSceneTextures) override;

   /// Called right before Post Processing rendering begins
   virtual void PrePostProcessPass_RenderThread(FRDGBuilder& inGraphBuilder, const FSceneView& inView, const FPostProcessingInputs& inPostProcInputs) override;
};
