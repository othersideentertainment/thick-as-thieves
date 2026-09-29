// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ScreenPass/OSESceneViewExtension.h"


class OSERENDERER_API FOSEScreenPassSNN : public FOSESceneViewExtension
{
public:

   FOSEScreenPassSNN(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem);

   // Called on game thread when view family is about to be rendered
   virtual void BeginRenderViewFamily(FSceneViewFamily& inViewFamily) override;

   // Called on render thread at the start of rendering
   virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& inRHICmdList, FSceneViewFamily& inViewFamily) override;

   // Called right before Post Processing rendering begins
   virtual void PrePostProcessPass_RenderThread(FRDGBuilder& inGraphBuilder, const FSceneView& inView, const FPostProcessingInputs& inPostProcInputs) override;

   // This will be called at the beginning of post processing to make sure that each view extension gets a chance to subscribe to an after pass event
   virtual void SubscribeToPostProcessingPass(EPostProcessingPass inPass, FAfterPassCallbackDelegateArray& inOutPassCallbacks, bool inIsPassEnabled) override;

   // Callback to perform this after the tonemap occurs
   //FScreenPassTexture PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& inGraphBuilder, const FSceneView& inView, const FPostProcessMaterialInputs& inPostProcInputs);
};
