// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/TargetActors/OSEAbilityTargetActor_BaseTrace.h"

#include "OSEAbilityTargetActor_ConeTrace.generated.h"


// A cone trace that returns closest valid pawn
// TODO: optional hysteresis?
// TODO: fuzzy distance+angle heuristic for closeness? (possibly fine with cone)
UCLASS()
class OSECORE_API AOSEAbilityTargetActor_ConeTrace : public AOSEAbilityTargetActor_BaseTrace
{
   GENERATED_BODY()

public:
   AOSEAbilityTargetActor_ConeTrace();

   // The half angle of the cone in degrees
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   float HalfAngle;

protected:
   virtual FHitResult PerformTrace(AActor* inSourceActor) override;

   virtual FCollisionObjectQueryParams MakeObjectQueryParams() const;
   virtual FCollisionProfileName GetTraceProfile() const;
};
