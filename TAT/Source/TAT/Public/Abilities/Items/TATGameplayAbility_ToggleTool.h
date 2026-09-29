// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_ToggleTool.generated.h"

class AOSECharacterBase;
class UToolComponent;

// Toggle a specific specific tool in our toolset by type
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class TAT_API UTATGameplayAbility_ToggleTool : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UTATGameplayAbility_ToggleTool();

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UToolComponent> ToolClass;

   // from UGameplayAbility
   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
};

