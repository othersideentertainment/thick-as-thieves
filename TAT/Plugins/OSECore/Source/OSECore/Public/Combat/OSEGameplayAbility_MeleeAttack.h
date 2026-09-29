// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue4

#include "OSEGameplayAbility_MeleeAttack.generated.h"

// TODO: It's possible that this impl is too TAT-specific and should live at the project level

class AOSECharacterBase;
class UCombatComponent;

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_MeleeAttack : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UOSEGameplayAbility_MeleeAttack();

   float GetChainAttackInputBufferWindow() const { return ChainAttackInputBufferWindow; }

   // from UGameplayAbility
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;

protected:
   // This is the time before the "combat end" anim notify that we accept input to automatically chain the next attack
   UPROPERTY(EditDefaultsOnly, Category = "Combat");
   float ChainAttackInputBufferWindow = 0.1f;

protected:
   AOSECharacterBase& _GetCharacter() const;
   UCombatComponent& _GetCombatComponent() const;

private:
   float _abilityActivateWorldTime = 0.0f;
};
