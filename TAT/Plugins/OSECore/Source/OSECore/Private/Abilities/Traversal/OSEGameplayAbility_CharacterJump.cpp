// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterJump.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterJump)


UOSEGameplayAbility_CharacterJump::UOSEGameplayAbility_CharacterJump() : Super()
{
   // The underlying functionality is implemented in terms of the character.
   // The ability that calls it does not need replication to the server,
   // as it is handled already.
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UOSEGameplayAbility_CharacterJump::CanActivateAbility(ACharacter* character) const
{
   check(character != nullptr);
   return character->CanJump();
}

void UOSEGameplayAbility_CharacterJump::ActivateAbility(ACharacter* character)
{
   check(character != nullptr);
   character->Jump();
}

void UOSEGameplayAbility_CharacterJump::CancelAbility(ACharacter* character)
{
   check(character != nullptr);
   character->StopJumping();
}

