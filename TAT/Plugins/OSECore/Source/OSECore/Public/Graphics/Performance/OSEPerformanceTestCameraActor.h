// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Camera/CameraActor.h"

#include "OSEPerformanceTestCameraActor.generated.h"

class APlayerController;

UCLASS(Blueprintable)
class OSECORE_API AOSEPerformanceTestCameraActor : public ACameraActor
{
   GENERATED_BODY()

public:

   AOSEPerformanceTestCameraActor(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // from ACameraActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void BecomeViewTarget(APlayerController* pc) override;
   virtual void EndViewTarget(APlayerController* pc) override;
   virtual void Tick(float deltaSeconds) override;

   void StartTesting();
   void StopTesting();
   
   UPROPERTY(EditAnywhere, Category = "OSE Performance Test")
   FString PerformanceTestCameraName;

   UPROPERTY(EditAnywhere, Category = "OSE Performance Test")
   bool AllowRotation = true;

protected:
#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif

   FRotator _originalRotation;
};
