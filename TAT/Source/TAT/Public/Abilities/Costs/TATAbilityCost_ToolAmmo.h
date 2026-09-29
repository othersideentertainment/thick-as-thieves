// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityCost.h"

// ue5
#include "GameplayTagContainer.h"

#include "TATAbilityCost_ToolAmmo.generated.h"

class UTATToolComponent;

UENUM()
enum class ETATToolCostType : uint8
{
   /// Check and use ammo against whatever our source/granting tool is
   SourceTool,

   /// Check and use ammo against a specific tool of a given category
   SpecificToolCategory
};

UCLASS()
class TAT_API UTATAbilityCost_ToolAmmo : public UOSEAbilityCost
{
   GENERATED_BODY()
public:

   virtual bool CanAfford(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, FGameplayTagContainer* optionalRelevantTags) const override;
   virtual void ApplyCost(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) const override;

   UPROPERTY(EditAnywhere)
   ETATToolCostType ToolCostType = ETATToolCostType::SourceTool;

   UPROPERTY(EditAnywhere, Meta = (EditCondition="ToolCostType == ETATToolCostType::SpecificToolCategory", EditConditionHides))
   FGameplayTag ToolCategoryToCostAmmo;

   UPROPERTY(EditAnywhere)
   int32 AmmoCost = 1;

   /// If true, we will gate ability activation on being able to afford the cost, but will not actually apply it when the ability fires
   /// Useful for preliminary abilities that should be prevented by lack of ammo
   UPROPERTY(EditAnywhere)
   bool CheckOnly = false;

private:
   UTATToolComponent* _GetAssociatedTool(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo) const;
};


