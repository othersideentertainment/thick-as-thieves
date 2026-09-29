// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEGameplayAbility.h"

#include "ToolRangedWeaponAbilityBase.generated.h"

class UToolRangedWeaponComponent;

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UToolRangedWeaponAbilityBase : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UToolRangedWeaponAbilityBase();
   
   // from UGameplayAbility
   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;
   virtual bool CommitAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) override;
   virtual bool CommitAbilityCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) override;

protected:
   // for our subclasses
   virtual bool CanActivateAbility(const UToolRangedWeaponComponent& rangedWeaponComp) const { return true; }
   virtual void ActivateAbility(UToolRangedWeaponComponent& rangedWeaponComp) { }
   virtual void EndAbility(UToolRangedWeaponComponent& rangedWeaponComp) { }
   virtual void CommitAbility(UToolRangedWeaponComponent& rangedWeaponComp) { }
   virtual void CommitAbilityCost(UToolRangedWeaponComponent& rangedWeaponComp) { }

protected:
   UFUNCTION(BlueprintPure, Category = "Weapon")
   UToolRangedWeaponComponent* GetRangedWeaponComponent() const;
   UToolRangedWeaponComponent* _GetRangedWeaponComponent(const FGameplayAbilityActorInfo* actorInfo) const;
};
