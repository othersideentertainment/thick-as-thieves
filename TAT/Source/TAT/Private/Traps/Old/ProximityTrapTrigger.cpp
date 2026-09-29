// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/Old/ProximityTrapTrigger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ProximityTrapTrigger)

void AProximityTrapTrigger_Old::NotifyActorBeginOverlap(AActor* otherActor)
{
   if (!HasAuthority() || !CanBeTriggeredByActor(otherActor)) return;

   if (IsInState(ETrapTriggerState::Ready))
   {
      AuthorityTrigger();
   }
}

