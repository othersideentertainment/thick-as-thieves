// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

#include "OSEGameplayAbility_EquipToolBase.generated.h"

class AOSECharacterBase;
class UToolComponent;

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_EquipToolBase : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UOSEGameplayAbility_EquipToolBase();

   // from UGameplayAbility
   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;

protected:
   // to override in subclasses
   virtual TSubclassOf<UToolComponent> _GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const;
   virtual void _OnToolClassEquipped(const FGameplayAbilityActorInfo* actorInfo, const TSubclassOf<UToolComponent>& toolClass) { }
  
   // utl
   AOSECharacterBase* _GetCharacter(const FGameplayAbilityActorInfo* actorInfo) const;
};
