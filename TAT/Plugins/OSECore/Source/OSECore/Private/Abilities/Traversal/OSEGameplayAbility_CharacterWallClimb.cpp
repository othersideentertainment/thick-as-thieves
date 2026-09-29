// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterWallClimb.h"

// OSE
#include "Traversal/TraversalInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterWallClimb)


UOSEGameplayAbility_CharacterWallClimb::UOSEGameplayAbility_CharacterWallClimb() : Super()
{
   // The underlying functionality is implemented in terms of the character.
   // The ability that calls it does not need replication to the server,
   // as it is handled already.
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UOSEGameplayAbility_CharacterWallClimb::CanActivateAbility(ACharacter* character) const
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->CanWallClimb();
   }

   return false;
}

void UOSEGameplayAbility_CharacterWallClimb::ActivateAbility(ACharacter* character)
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->StartWallClimbing();
   }
}

void UOSEGameplayAbility_CharacterWallClimb::CancelAbility(ACharacter* character)
{
   if (auto traversal = Cast<ITraversalInterface>(character))
   {
      return traversal->StopWallClimbing();
   }
}

