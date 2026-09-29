// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/TATProximityTrap.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATProximityTrap)

void ATATProximityTrap::NotifyActorBeginOverlap(AActor* otherActor)
{
   if (!HasAuthority() || !CanBeTriggeredByActor(otherActor)) return;

   if (IsInState(ETATTrapState::Ready))
   {
      AuthorityTrigger(otherActor);
   }
}

