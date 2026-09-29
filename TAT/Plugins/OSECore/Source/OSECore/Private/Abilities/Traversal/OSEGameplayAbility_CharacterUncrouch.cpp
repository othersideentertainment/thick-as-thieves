// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterUncrouch.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterUncrouch)

bool UOSEGameplayAbility_CharacterUncrouch::CanActivateAbility(ACharacter* character) const
{
   check(character != nullptr);
   if (auto movementComp = character->GetCharacterMovement())
   {
      if (movementComp->IsCrouching() && movementComp->CanEverCrouch())
      {
         return true;
      }
   }
   return false;
}

void UOSEGameplayAbility_CharacterUncrouch::ActivateAbility(ACharacter* character)
{
   check(character != nullptr);
   if (auto movementComp = character->GetCharacterMovement())
   {
      character->UnCrouch();
   }
}

