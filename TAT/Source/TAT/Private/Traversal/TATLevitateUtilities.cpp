// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/TATLevitateUtilities.h"

// ose
#include "Character/TATCharacterMovement.h"

// ue4
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLevitateUtilities)

void UTATLevitateUtilities::AttachToLevitatingBase(UMovementComponent* movement, const AActor* base)
{
   UTATCharacterMovement* cmc = Cast<UTATCharacterMovement>(movement);
   check(cmc);

   if (ensure(base))
   {
      UPrimitiveComponent* baseRoot = CastChecked<UPrimitiveComponent>(base->GetRootComponent());

      cmc->SetTATCustomMovementType(ETATCustomMovementType::LevitatePassenger);
      cmc->SetBase(baseRoot, NAME_None, false);
   }
}

void UTATLevitateUtilities::DetachFromLevitatingBase(UMovementComponent* movement)
{
   if (UTATCharacterMovement* cmc = Cast<UTATCharacterMovement>(movement))
   {
      if(cmc->IsLevitatePassenger())
      {
         // falling is safe to return to
         cmc->SetMovementMode(MOVE_Falling);
      }
   }
}

