// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "OSERenderSubsystem.generated.h"


UCLASS()
class OSERENDERER_API UOSERenderSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:

   // Subsystem implementation
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

public:

   // Used to synchronize access across game / render thread
   FCriticalSection& GetCriticalSection() { return _criticalSection; }

private:

   FCriticalSection _criticalSection;

   // Post process scene extension (NPR)
   TSharedPtr < class FOSEScreenPassNPR, ESPMode::ThreadSafe > _sceneExtScreenPassNPR;

   // Post process scene extension (SNN)
   TSharedPtr < class FOSEScreenPassSNN, ESPMode::ThreadSafe > _sceneExtScreenPassSNN;

   // Post process scene extension (Custom Materials)
   TSharedPtr < class FOSECustomPostProc, ESPMode::ThreadSafe > _sceneExtCustomPostProc;
};
