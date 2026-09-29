// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/SwingingDoorNavLinkComponent.h"

#include "Interactables/TATSwingingDoor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SwingingDoorNavLinkComponent)

USwingingDoorNavLinkComponent::USwingingDoorNavLinkComponent()
{
   LinkRelativeStart = FVector(0, -80, 0);
   LinkRelativeEnd = FVector(0, 80, 0);
}

bool USwingingDoorNavLinkComponent::IsLinkPathfindingAllowed(const UObject* querier) const
{
   if (const auto door = Cast<ATATSwingingDoor>(GetOwner()))
   {
      return door->CanTraverseDoor(querier, LinkDirection == ENavLinkDirection::RightToLeft);
   }

   return false;
}
