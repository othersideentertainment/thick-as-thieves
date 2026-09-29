// (c) 2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "ScreenPass/OSESceneViewExtension.h"
#include "UObject/GCObject.h"
#include "OSECustomPostProc.generated.h"


UCLASS()
class OSERENDERER_API UCustomPPMaterialContainer : public UObject
{
   GENERATED_BODY()

public:
   
   void Load();

   void Reset() { _materialInstances.Reset(); }

   UPROPERTY(Transient, DuplicateTransient)
   TArray<UMaterialInstanceDynamic*> _materialInstances;
};


class OSERENDERER_API FOSECustomPostProc : public FOSESceneViewExtension, FGCObject
{
public:

   FOSECustomPostProc(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem);
   virtual ~FOSECustomPostProc();

   virtual void SetupView(FSceneViewFamily& inViewFamily, FSceneView& inView) override;

   virtual void BeginRenderViewFamily(FSceneViewFamily& /*inViewFamily*/) override { }

public:

   // FGCObject Interface
   virtual void AddReferencedObjects(FReferenceCollector& inCollector) override;
   virtual FString GetReferencerName() const { return TEXT("FOSECustomPostProc"); }

private:

   TObjectPtr<UCustomPPMaterialContainer> _ppMaterials = nullptr;
};
