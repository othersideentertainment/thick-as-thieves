// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "OSEGameplayAbility_SyncedAnimationEvent.generated.h"


class UOSESyncedAnimationDataAsset;

// a stripped-down ability that triggers synced animations from a gameplay event
// Synced animations still have a lot of cruft, but this strips down part of it (and is non-instanced)
UCLASS()
class UOSEGameplayAbility_SyncedAnimationEvent : public UOSEGameplayAbility
{
   GENERATED_BODY()

   UOSEGameplayAbility_SyncedAnimationEvent();

   // from UGameplayAbility
   virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* actorInfo, const FGameplayEventData* payload) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

protected:
   bool _MeetsConstraints(const AActor* sourceActor, const AActor* targetActor) const;

   // hook so blueprint can do some extra stuff in a non-instanced ability
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnTriggered", meta = (ScriptName = "OnTriggered"))
   void BP_OnTriggered(const AActor* source, const AActor* target, const FGameplayAbilityActorInfo& actorInfo) const;

   // conditional predicate for skipping source animation
   UFUNCTION(BlueprintImplementableEvent, meta = (ScriptName = "ShouldPlaySourceAnimation"))
   bool BP_ShouldSkipSourceAnimation(const AActor* source, const AActor* target) const;

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Synced Animation")
   UOSESyncedAnimationDataAsset* SyncedAnimationDataAsset = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Synced Animation", AdvancedDisplay)
   FGameplayTag PlayerEventTag;
	
   // Whether to skip playing the target animation
   UPROPERTY(EditDefaultsOnly, Category = "Synced Animation")
   bool SkipTarget;

   // Whether to skip playing the source animation
   UPROPERTY(EditDefaultsOnly, Category = "Synced Animation")
   bool SkipSource;
};
