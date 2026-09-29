// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/DetectionEnums.h"

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "DetectionTargetInterface.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UAIDetectionTargetInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API IAIDetectionTargetInterface
{
   GENERATED_BODY()

public:
   virtual void AuthorityOnDetectionStateChanged(AActor* detector, EActorDetectionState previousState, EActorDetectionState detectionState) = 0;
   virtual void AuthorityOnDetectionValueUpdateForState(AActor* detector, EActorDetectionState detectionState, float detectionValue) = 0;
};

USTRUCT()
struct TAT_API FAIDetectionState
{
   GENERATED_BODY()

   FAIDetectionState() = default;

   FAIDetectionState(const AActor* detector)
      : Detector(detector)
   {
   }

   UPROPERTY()
   TWeakObjectPtr<const AActor> Detector = nullptr;

   UPROPERTY()
   float DetectionValue = 0.0f;

   UPROPERTY()
   EActorDetectionState DetectionState = EActorDetectionState::Observing;

   FORCEINLINE bool operator==(const FAIDetectionState& other) const
   {
      return Detector == other.Detector;
   }
   FORCEINLINE bool operator==(const AActor* actor) const
   {
      return Detector.Get() == actor;
   }
};
