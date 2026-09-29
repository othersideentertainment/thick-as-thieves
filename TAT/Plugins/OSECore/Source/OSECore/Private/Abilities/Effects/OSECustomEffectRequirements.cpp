// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Effects/OSECustomEffectRequirements.h"

// ue5
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECustomEffectRequirements)

bool UOSEEffectRequirement_AvatarClass::CanApplyGameplayEffect_Implementation(const UGameplayEffect* gameplayEffect, const FGameplayEffectSpec& spec, UAbilitySystemComponent* asc) const
{
   const AActor* avatar = asc->GetAvatarActor();
   return avatar != nullptr && avatar->IsA(RequiredAvatarClass);
}

UOSEEffectRequirement_CharacterAvatar::UOSEEffectRequirement_CharacterAvatar()
{
   RequiredAvatarClass = ACharacter::StaticClass();
}
