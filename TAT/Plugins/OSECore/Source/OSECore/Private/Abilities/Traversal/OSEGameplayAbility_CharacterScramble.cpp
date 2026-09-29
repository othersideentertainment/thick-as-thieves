// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterScramble.h"

// OSE
#include "Traversal/TraversalInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterScramble)


UOSEGameplayAbility_CharacterScramble::UOSEGameplayAbility_CharacterScramble() : Super()
{
   // The underlying functionality is implemented in terms of the character.
   // The ability that calls it does not need replication to the server,
   // as it is handled already.
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UOSEGameplayAbility_CharacterScramble::CanActivateAbility(ACharacter* character) const
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->CanScramble();
   }

   return false;
}

void UOSEGameplayAbility_CharacterScramble::ActivateAbility(ACharacter* character)
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->StartScrambling();
   }
}

void UOSEGameplayAbility_CharacterScramble::CancelAbility(ACharacter* character)
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->StopScrambling();
   }
}

