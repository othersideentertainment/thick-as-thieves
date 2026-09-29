// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayAbilitySpec.h"

#include "OSEAbilityCost.generated.h"


// A class for ability costs that cannot be represented just by a gameplay effect (and default affordability checks)
UCLASS(Abstract, EditInlineNew)
class OSECORE_API UOSEAbilityCost : public UObject
{
   GENERATED_BODY()

public:
   // Should I just call this CheckCost, since this is just a passthrough?
   virtual bool CanAfford(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, FGameplayTagContainer* optionalRelevantTags) const
   {
      return true;
   }

   virtual void ApplyCost(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) const {}
};
