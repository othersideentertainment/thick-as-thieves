// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Disguise/TATDisguiseProgressSource.h"

// tat
#include "Disguise/TATDisguisableCharacterInterface.h"
#include "Disguise/TATDisguiseComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDisguiseProgressSource)

float UTATDisguiseProgressSource::GetProgress(const AActor* character) const
{
   if(character && character->Implements<UTATDisguisableCharacterInterface>())
   {
      if(const UTATDisguiseComponent* disguiseComponent = ITATDisguisableCharacterInterface::Execute_GetDisguiseComponent(character))
      {
         return disguiseComponent->GetNormalizedRemainingDisguiseIntegrity();
      }
   }

   return 0.0f;
}
