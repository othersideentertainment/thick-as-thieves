// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterCrouch.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterCrouch)

bool UOSEGameplayAbility_CharacterCrouch::CanActivateAbility(ACharacter* character) const
{
   check(character != nullptr);
   if (auto movementComp = character->GetCharacterMovement())
   {
      if (movementComp->CanEverCrouch())
      {
         return (UnCrouchWhenCancelled || !movementComp->IsCrouching());
      }
   }

   return false;   
}

void UOSEGameplayAbility_CharacterCrouch::ActivateAbility(ACharacter* character)
{   
   check(character != nullptr);
   if (auto movementComp = character->GetCharacterMovement())
   {
      if (!movementComp->IsCrouching())
         character->Crouch();
   }
}

void UOSEGameplayAbility_CharacterCrouch::CancelAbility(ACharacter* character)
{
   check(character != nullptr);
   if (auto movementComp = character->GetCharacterMovement())
   {
      if (!movementComp->IsMoveInputIgnored())
      {
         if (UnCrouchWhenCancelled && movementComp->IsCrouching())
            character->UnCrouch();
      }
   }
}

