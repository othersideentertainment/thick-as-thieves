// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/TargetActors/OSEAbilityTargetActor_BaseTrace.h"

// ue
#include "CoreMinimal.h"

#include "OSEAbilityTargetActor_PathTrace.generated.h"

///
/// Path targeting actor, used for finding two valid points, a start and end location that have line of sight to each other.
///
UCLASS()
class OSECORE_API AOSEAbilityTargetActor_PathTrace : public AOSEAbilityTargetActor_BaseTrace
{
   GENERATED_BODY()

public:
   AOSEAbilityTargetActor_PathTrace();

protected:
   virtual FHitResult PerformTrace(AActor* inSourceActor) override;
   virtual void ConfirmTargetingAndContinue() override;

   /// Trace from the path's start location to a target end location and make sure it has clear line of sight
   virtual bool _PerformPathTrace(FHitResult& outHitResult, const FVector& targetLocation) const;

   virtual bool _AllowHitAsTarget(const FHitResult& hit) const;

   // pre-quantize the hit
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Trace)
   bool bPreQuantizeHitResult = false;

   /// The start location of the path.
   /// Two traces will be done - one from this start location and one from the source actor's viewpoint.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Trace)
   FVector PathStartWorldLocation = FVector::ZeroVector;

   // From AActor
   virtual void Tick(float deltaSeconds) override;

   UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
   void UpdateTargetingVisuals(const FHitResult& targetHitResult, bool isValidTarget, const FHitResult& pathHitResult, bool isValidPath);
};
