// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_UnequipCurrentTool.generated.h"

/// Ability class that unequips the currently-equipped tool in our toolset
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class TAT_API UTATGameplayAbility_UnequipCurrentTool : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UTATGameplayAbility_UnequipCurrentTool();

   // from UGameplayAbility
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
};
