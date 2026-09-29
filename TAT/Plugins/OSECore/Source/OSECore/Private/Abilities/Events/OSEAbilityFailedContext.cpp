// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Events/OSEAbilityFailedContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityFailedContext)

bool FOSEAbilityFailedContext::NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess)
{
   if (!Super::NetSerialize(ar, map, outSuccess))
   {
      return false;
   }
   if (!FailureTags.NetSerialize(ar, map, outSuccess))
   {
      return false;
   }
   outSuccess = true;
   return true;
}

