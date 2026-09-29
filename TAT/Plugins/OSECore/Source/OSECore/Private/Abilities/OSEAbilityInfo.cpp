// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEAbilityInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityInfo)

float UOSEAbilityProgressSource::EvaluateProgress(TSubclassOf<UOSEAbilityProgressSource> source, const AActor* character)
{
   if (!source)
   {
      return 0;
   }

   return source.GetDefaultObject()->GetProgress(character);
}
