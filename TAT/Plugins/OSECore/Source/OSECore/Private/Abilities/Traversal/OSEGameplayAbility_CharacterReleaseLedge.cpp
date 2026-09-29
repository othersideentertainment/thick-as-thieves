// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterReleaseLedge.h"
#include "Character/OSECharacterBase.h"
#include "Character/OSECharacterMovement.h"



#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterReleaseLedge)

bool UOSEGameplayAbility_CharacterReleaseLedge::CanActivateAbility(ACharacter* character) const
{
   check(character != nullptr);
   if (UOSECharacterMovement* movementComp = Cast< UOSECharacterMovement>(character->GetCharacterMovement()))
   {
      if (movementComp->IsInLedgeState())
      {
         return true;
      }
   }

   return false;   
}

void UOSEGameplayAbility_CharacterReleaseLedge::ActivateAbility(ACharacter* character)
{   
   check(character != nullptr);

   if (AOSECharacterBase* oseCharacter = Cast<AOSECharacterBase>(character))
   {
      oseCharacter->StartReleaseLedge();
   }
}

void UOSEGameplayAbility_CharacterReleaseLedge::CancelAbility(ACharacter* character)
{
   check(character != nullptr);

   if (AOSECharacterBase* oseCharacter = Cast<AOSECharacterBase>(character))
   {
      oseCharacter->StopReleaseLedge();
   }
}

