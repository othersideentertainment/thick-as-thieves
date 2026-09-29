// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Combat/OSEGameplayAbility_MeleeAttack.h"

// ue
#include "GameplayTagContainer.h"

#include "TATGameplayAbility_MeleeAttack.generated.h"

class UTATMeleeWeaponToolComponent;

UCLASS(Abstract)
class TAT_API UTATGameplayAbility_MeleeAttack : public UOSEGameplayAbility_MeleeAttack
{
   GENERATED_BODY()

   // from UGameplayAbility
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

protected:
   UPROPERTY(Transient, BlueprintReadOnly)
   FGameplayTag _toolUsageTag;
};
