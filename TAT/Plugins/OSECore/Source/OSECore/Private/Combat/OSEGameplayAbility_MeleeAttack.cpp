// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/OSEGameplayAbility_MeleeAttack.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Combat/CombatComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_MeleeAttack)

// ue4

DEFINE_LOG_CATEGORY_STATIC(LogOSEGameplayAbilityMeleeAttack, Log, Log);

UOSEGameplayAbility_MeleeAttack::UOSEGameplayAbility_MeleeAttack()
   : Super()
{

}

void UOSEGameplayAbility_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);
   const float now = GetWorld()->GetTimeSeconds();
   _abilityActivateWorldTime = now;
   UE_LOG(LogOSEGameplayAbilityMeleeAttack, Verbose, TEXT("%s: Activate Ability At World Time %.02f"), *GetName(), now);

   UCombatComponent& combatComponent = _GetCombatComponent();
   combatComponent.OnCombatMeleeAttackAbilityStart(*this);
}

void UOSEGameplayAbility_MeleeAttack::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
   const float now = GetWorld()->GetTimeSeconds();
   const float attackDuration = now - _abilityActivateWorldTime;
   UE_LOG(LogOSEGameplayAbilityMeleeAttack, Verbose, TEXT("%s: End Ability At World Time %.02f"), *GetName(), now);
   UE_LOG(LogOSEGameplayAbilityMeleeAttack, Verbose, TEXT("%s: Attack Duration %.02f"), *GetName(), attackDuration);
}

AOSECharacterBase& UOSEGameplayAbility_MeleeAttack::_GetCharacter() const
{
   const FGameplayAbilityActorInfo* actorInfo = GetCurrentActorInfo();
   check(actorInfo);
   return *CastChecked<AOSECharacterBase>(actorInfo->AvatarActor.Get());
}

UCombatComponent& UOSEGameplayAbility_MeleeAttack::_GetCombatComponent() const
{
   AOSECharacterBase& character = _GetCharacter();
   UCombatComponent* combatComp = character.GetCombatComponent();
   check(combatComp);
   return *combatComp;
}

