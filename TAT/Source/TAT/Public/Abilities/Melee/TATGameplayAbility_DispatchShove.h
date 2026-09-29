// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue
#include "GameplayTagContainer.h"

#include "TATGameplayAbility_DispatchShove.generated.h"

class UAnimMontage;

/// Local-only ability that activates a shove "attack" ability
UCLASS()
class TAT_API UTATGameplayAbility_DispatchShove : public UOSEGameplayAbility
{
   GENERATED_BODY()

   UTATGameplayAbility_DispatchShove();

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From UGameplayAbility
   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

private:
   // Returns a random entry from _shoveMontages
   const UAnimMontage* _SelectShoveMontage() const;

private:
   // Collection of montages randomly selected when performing a shove
   UPROPERTY(EditDefaultsOnly)
   TArray<UAnimMontage*> _shoveMontages;

   // Gameplay event to trigger the shove ability
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag _shoveAbilityGameplayEvent;
};
