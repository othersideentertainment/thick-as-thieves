// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_SyncedDispatch.generated.h"

/// A stub ability to dispatch the provided event to both the source and the target. It solely exists as a synchronization point so client and server do not diverge
UCLASS()
class TAT_API UTATGameplayAbility_SyncedDispatch : public UOSEGameplayAbility
{
	GENERATED_BODY()
   
   UTATGameplayAbility_SyncedDispatch();

   // from UGameplayAbility
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

protected:
   // hook so blueprint can do some extra stuff in a non-instanced ability
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnTriggered", meta = (ScriptName = "OnTriggered"))
   void BP_OnTriggered(const AActor* source, const AActor* target, const FGameplayAbilityActorInfo& actorInfo) const;

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Synced Animation", AdvancedDisplay)
   FGameplayTag PlayerEventTag;
	
};
