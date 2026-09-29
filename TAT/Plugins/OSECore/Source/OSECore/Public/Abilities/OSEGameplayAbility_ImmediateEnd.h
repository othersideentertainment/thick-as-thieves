// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEGameplayAbility.h"
#include "OSEGameplayAbility_ImmediateEnd.generated.h"


/// A gameplay ability that ends itself immediately in C++
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_ImmediatateEnd : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:

   UOSEGameplayAbility_ImmediatateEnd();

   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
};
