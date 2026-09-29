// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"


class UOSERenderSubsystem;


class OSERENDERER_API FOSESceneViewExtension : public FSceneViewExtensionBase
{
public:
   
   FOSESceneViewExtension(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem);
   virtual ~FOSESceneViewExtension();

public:

   /// Called on game thread when creating the view family.
   virtual void SetupViewFamily(FSceneViewFamily& inViewFamily) override { };

   /// Called on game thread when creating the view.
   virtual void SetupView(FSceneViewFamily& inViewFamily, FSceneView& inView) override { };

   /// Called on game thread when view family is about to be rendered
   virtual void BeginRenderViewFamily(FSceneViewFamily& inViewFamily) override { };

protected:

   // Returns the render subsystem
   const UOSERenderSubsystem* GetRenderSubsystem() const { return _renderSubsystem.Get(); }
   UOSERenderSubsystem* GetRenderSubsystem() { return _renderSubsystem.Get(); }

   // Returns the world associated with the render subsystem
   const UWorld* GetWorld() const;

private:

   TWeakObjectPtr< UOSERenderSubsystem > _renderSubsystem = nullptr;
};
