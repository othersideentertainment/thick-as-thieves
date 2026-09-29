// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Abilities/GameplayAbilityTargetActor_GroundTrace.h"

#include "OSEAbilityTargetActor_GroundTrace.generated.h"

UCLASS()
class OSECORE_API AOSEAbilityTargetActor_GroundTrace : public AGameplayAbilityTargetActor_GroundTrace
{
   GENERATED_BODY()

public:
   AOSEAbilityTargetActor_GroundTrace();

   // Ensure the target we place on the ledge is visible
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Targeting)
   bool EnsureTargetVisibility = true;

   // Allow targeting on ledges
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Targeting)
   bool AllowTargetingOnLedges = true;

   // Ignore overlaps, only hit blocks
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Targeting)
   bool IgnoreOverlaps = true;

   virtual bool ShouldProduceTargetData() const override;

protected:
   virtual void ConfirmTargeting() override;
   virtual void CancelTargeting() override;
   virtual void StartTargeting(UGameplayAbility* inAbility) override;
   virtual FHitResult PerformTrace(AActor* inSourceActor) override;
   bool AdjustCollisionResultForShapeWithVisibility(const FVector originalStartPoint, const FVector originalEndPoint, const FCollisionQueryParams params, FHitResult& outHitResult) const;
};
