// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

// tat 
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "Abilities/TATGameplayTags.h"

// ose
#include "Character/OSETeamInterface.h"
#include "Items/OSEProjectile.h"

#include "TATProjectile.generated.h"

//
// A base projectile class for TAT specific implementations.
// 
// Entire class (specifically gravity replication) is subject to change.
//
UCLASS()
class TAT_API ATATProjectile : public AOSEProjectile, public IOSETeamInterface, public ITATUtilityAITargetingGroupInterface
{
	GENERATED_BODY()
public:

   // from AActor
   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


   // begin IOSETeamInterface
   virtual uint8 GetTeam() const override;
   // end IOSETeamInterface

   // begin ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
   // end ITATUtilityAITargetingGroupInterface
protected:
   // the gravity scale for the movement component to use.
   // this was implemented originally for the differing gravity between spirit realm and the physical realm.
   // that way a projectile can differ their gravity according to the realm they are in.
   // 
   // potentially changing after full projectile overhaul.
   UPROPERTY(Replicated, BlueprintReadOnly, EditAnywhere, meta = (ExposeOnSpawn))
   float InitialReplicatedGravityScale = 1.f;
   
   UPROPERTY(EditAnywhere, Category = Projectile)
   FGameplayTag _ProjectileTargetingGroup { TAG_AI_TargetingGroup_Projectile };
};
