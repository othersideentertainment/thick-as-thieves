// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "Abilities/TargetActors/OSEAbilityTargetActor_AOE.h"

// ue4
#include "Engine/CollisionProfile.h"

#include "OSEAbilityTargetActor_ConeAOE.generated.h"

class UGameplayAbility;

UCLASS(Blueprintable, notplaceable)
class OSECORE_API AOSEAbilityTargetActor_ConeAOE : public AOSEAbilityTargetActor_AOE
{
   GENERATED_BODY()

public:
   AOSEAbilityTargetActor_ConeAOE(const FObjectInitializer&);

   // The maximum distance from origin of the code
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   float MaxRange;

   // The half angle of the cone in degrees
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   float HalfAngle;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   bool bCheckLineOfSight;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true, EditCondition="bCheckLineOfSight"))
   FCollisionProfileName LineOfSightTraceProfile;

protected:
   virtual void PerformOverlap(TArray<TWeakObjectPtr<AActor>>& result, bool positionForPreview) override;
};
