// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_DodgeTrigger.generated.h"

class UInputAction;

// Triggers a dodge ability if move input is in a not-forward direction
UCLASS()
class TAT_API UTATGameplayAbility_DodgeTrigger : public UOSEGameplayAbility
{
   GENERATED_BODY()
   

public:
   UTATGameplayAbility_DodgeTrigger();

   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

public:
   UPROPERTY(EditDefaultsOnly, Category = "Dodge Settings")
   bool AllowForwardsDodge = true;

   // How far to the left/right of the player-forward direction are we triggering for?
   UPROPERTY(EditDefaultsOnly, Category = "Dodge Settings", meta = (EditCondition = "!AllowForwardsDodge"))
   float AccelForwardHalfAngle = 65.0f;

   // The tag to use to trigger the actual dodge
   UPROPERTY(EditDefaultsOnly, Category = "Dodge Settings")
   FGameplayTag DodgeEventTag;
};
