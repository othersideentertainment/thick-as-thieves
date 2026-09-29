// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_SyncedSelect.generated.h"

class UOSESyncedAnimationDataAsset;


/// A simple ability that will try to dispatch the first matching synced animation it finds
/// Initially for use by directional takedowns
/// We may want o hoist more up out of the anim data asset to reduce workflow and perf issues from duplication
UCLASS()
class TAT_API UTATGameplayAbility_SyncedSelect : public UOSEGameplayAbility
{
	GENERATED_BODY()

public:
   UTATGameplayAbility_SyncedSelect();

   // from UGameplayAbility
   virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* actorInfo, const FGameplayEventData* payload) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

   // TODO: move elsewhere
   static const UOSESyncedAnimationDataAsset* FindMatchingAnimation(const TArray<UOSESyncedAnimationDataAsset*>& animOptions, const AActor* sourceActor, const AActor* targetActor);


protected:
   UPROPERTY(EditDefaultsOnly, Category = "Synced Animation")
   TArray<UOSESyncedAnimationDataAsset*> SyncedAnimationAssets;

   UPROPERTY(EditDefaultsOnly, Category = "Synced Animation", AdvancedDisplay)
   FGameplayTag DispatchEventTag;
};
