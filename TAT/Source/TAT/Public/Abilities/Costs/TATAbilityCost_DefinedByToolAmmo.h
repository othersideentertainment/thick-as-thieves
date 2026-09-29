// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityCost.h"

// ue5
#include "GameplayTagContainer.h"

#include "TATAbilityCost_DefinedByToolAmmo.generated.h"

class UTATToolComponent;
/**
 * Gets the cost for this ability by checking defined costs in the source tool via ToolUsageTag
 */
UCLASS()
class TAT_API UTATAbilityCost_DefinedByToolAmmo : public UOSEAbilityCost
{
   GENERATED_BODY()

   virtual bool CanAfford(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, FGameplayTagContainer* optionalRelevantTags) const override;
   virtual void ApplyCost(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) const override;

   UPROPERTY(EditAnywhere, meta = (Categories = "Tool.Usage"))
   FGameplayTag ToolUsageTag;

   /// If true, we will gate ability activation on being able to afford the cost, but will not actually apply it when the ability fires
   /// Useful for preliminary abilities that should be prevented by lack of ammo
   UPROPERTY(EditAnywhere)
   bool CheckOnly = false;

private:
   UTATToolComponent* _GetAssociatedTool(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo) const;
};
