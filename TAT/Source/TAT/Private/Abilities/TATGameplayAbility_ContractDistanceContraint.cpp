// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TATGameplayAbility_ContractDistanceContraint.h"

// ose
#include "Character/OSECharacterBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_ContractDistanceContraint)

UTATGameplayAbility_ContractDistanceContraint::UTATGameplayAbility_ContractDistanceContraint() : Super()
{
   // The underlying functionality is implemented in terms of the character.
   // The ability that calls it does not need replication to the server,
   // as it is handled already.
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UTATGameplayAbility_ContractDistanceContraint::CanActivateAbility(ACharacter* character) const
{
   if (auto traversal = Cast<AOSECharacterBase>(character))
   {
      return true;
   }

   return false;
}

void UTATGameplayAbility_ContractDistanceContraint::ActivateAbility(ACharacter* character)
{
   if (auto traversal = Cast<AOSECharacterBase>(character))
   {
      return traversal->ContractDistanceConstraintRequest();
   }
}

void UTATGameplayAbility_ContractDistanceContraint::CancelAbility(ACharacter* character)
{
   if (auto traversal = Cast<AOSECharacterBase>(character))
   {
      return traversal->ContractDistanceConstraintCancel();
   }
}
