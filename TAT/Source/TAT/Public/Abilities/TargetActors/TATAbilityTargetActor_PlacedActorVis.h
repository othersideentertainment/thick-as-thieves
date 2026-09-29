// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATToolWorldActorConstraint.h"

// ose
#include "Abilities/TargetActors/OSEAbilityTargetActor_LineTrace.h"

#include "TATAbilityTargetActor_PlacedActorVis.generated.h"

/// Shows a preview of where we are trying to place a tool's world actor,
/// and if it matches any constraints that have been set
UCLASS()
class TAT_API ATATAbilityTargetActor_PlacedActorVis : public AOSEAbilityTargetActor_LineTrace
{
	GENERATED_BODY()

public:
   ATATAbilityTargetActor_PlacedActorVis();

   // From AActor
   virtual void Tick(float deltaSeconds) override;

   UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
   void UpdateTargetingVisuals(const FHitResult& hitResult, bool isValidTarget);

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
   TSubclassOf<UTATToolWorldActorConstraint> WorldActorConstraint;

protected:

   // From AOSEAbilityTargetActor_LineTrace
   virtual bool _AllowHitAsTarget(const FHitResult& hit) override;
};
