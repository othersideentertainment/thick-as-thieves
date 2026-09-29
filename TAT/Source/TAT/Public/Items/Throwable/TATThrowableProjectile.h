// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Abilities/TATGameplayTags.h"
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"

// ose
#include "Items/InterpolatingProjectile.h"
#include "Character/OSETeamInterface.h"

#include "TATThrowableProjectile.generated.h"

// An actor used when a throwable is actually being thrown
UCLASS(NotPlaceable)
class TAT_API ATATThrowableProjectile : public AInterpolatingProjectile, public IOSETeamInterface, public ITATUtilityAITargetingGroupInterface
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ATATThrowableProjectile();

   // begin IOSETeamInterface
   virtual uint8 GetTeam() const override;
   // end IOSETeamInterface

   // begin ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
   // end ITATUtilityAITargetingGroupInterface

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

protected:
   UPROPERTY(BlueprintReadOnly, Replicated, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Projectile)
   FVector InitialVelocity { FVector::ZeroVector };
   
   UPROPERTY(EditAnywhere, Category = Projectile)
   FGameplayTag _ProjectileTargetingGroup { TAG_AI_TargetingGroup_Projectile };
};
