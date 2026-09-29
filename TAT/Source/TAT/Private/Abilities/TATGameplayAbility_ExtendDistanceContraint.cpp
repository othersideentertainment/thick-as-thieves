// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TATGameplayAbility_ExtendDistanceContraint.h"

// ose
#include "Character/OSECharacterBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_ExtendDistanceContraint)

UTATGameplayAbility_ExtendDistanceContraint::UTATGameplayAbility_ExtendDistanceContraint() : Super()
{
   // The underlying functionality is implemented in terms of the character.
   // The ability that calls it does not need replication to the server,
   // as it is handled already.
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UTATGameplayAbility_ExtendDistanceContraint::CanActivateAbility(ACharacter* character) const
{
   if (auto traversal = Cast<AOSECharacterBase>(character))
   {
      return true;
   }

   return false;
}

void UTATGameplayAbility_ExtendDistanceContraint::ActivateAbility(ACharacter* character)
{
   if (auto traversal = Cast<AOSECharacterBase>(character))
   {
      return traversal->ExtendDistanceConstraintRequest();
   }
}

void UTATGameplayAbility_ExtendDistanceContraint::CancelAbility(ACharacter* character)
{
   if (auto traversal = Cast<AOSECharacterBase>(character))
   {
      return traversal->ExtendDistanceConstraintCancel();
   }
}
