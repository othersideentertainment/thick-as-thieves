// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TATAbilityTargetActor_PlaceFront.generated.h"

// A target actor that tries to find a valid location for a capsule directly
// in front of the pawn (by view direction), offset vertically to account for
// floors/ramp/stairs.
// 
// It either finds a valid location, or nothing.
//
// Assumption: If there are LoS constraints, those are implicitly satisfied by
//             the target capsule overlapping that of the placing pawn.
UCLASS()
class TAT_API ATATAbilityTargetActor_PlaceFront : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:

   virtual void StartTargeting(UGameplayAbility* ability) override;
   virtual bool ShouldProduceTargetData() const override;

   virtual void ConfirmTargetingAndContinue() override;

private:
   FGameplayAbilityTargetDataHandle _PerformTargeting() const;

protected:
   // The 2d distance in front of the pawn that it will try to place the capsule
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Trace)
   float _forwardDistance;

   // The collision profile used to check for obstacles
   UPROPERTY(BlueprintReadWrite, EditAnywhere, config, meta = (ExposeOnSpawn = true), Category = Trace)
   FCollisionProfileName _collisionProfile;

   // How far upwards it will start sweeping down from the search for obstacles from below
   UPROPERTY(BlueprintReadWrite, EditAnywhere, config, meta = (ExposeOnSpawn = true), Category = Trace)
   float _downwardSweepStartOffset;

   // How much to pad the capsule radius when doing the downward sweep
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Trace)
   float _downwardSweepPadding;

   // The radius of the capsule that is being placed
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Trace)
   float _capsuleRadius;

   // The half-height of the capsule that is being places
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Trace)
   float _capsuleHalfHeight;
	
};
