// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterMantle.h"

// OSE
#include "Traversal/TraversalInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterMantle)


UOSEGameplayAbility_CharacterMantle::UOSEGameplayAbility_CharacterMantle() : Super()
{
   // The underlying functionality is implemented in terms of the character.
   // The ability that calls it does not need replication to the server,
   // as it is handled already.
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UOSEGameplayAbility_CharacterMantle::CanActivateAbility(ACharacter* character) const
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->CanMantle();
   }

   return false;
}

void UOSEGameplayAbility_CharacterMantle::ActivateAbility(ACharacter* character)
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->StartMantleAttempt();
   }
}

void UOSEGameplayAbility_CharacterMantle::CancelAbility(ACharacter* character)
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->StopMantleAttempt();
   }
}

