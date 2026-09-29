// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "Abilities/TargetActors/OSEAbilityTargetActor_AOE.h"

// ue4
#include "Engine/CollisionProfile.h"

#include "OSEAbilityTargetActor_TraceToSphereAOE.generated.h"

class UGameplayAbility;

UCLASS(Blueprintable, notplaceable)
class OSECORE_API AOSEAbilityTargetActor_TraceToSphereAOE : public AOSEAbilityTargetActor_AOE
{
   GENERATED_BODY()

public:
   AOSEAbilityTargetActor_TraceToSphereAOE(const FObjectInitializer&);

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   FCollisionProfileName TraceProfile;

   // The maximum distance that the sphere can be placed
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   float MaxRange;

   // The radius of the sphere
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   float SphereRadius;

   // The maximum number of targets. Will prioritize those closest to the sphere
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   int32 MaxTargets;

protected:
   virtual void PerformOverlap(TArray<TWeakObjectPtr<AActor>>& result, bool positionForPreview) override;
};
