// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATSecretDoorNavLinkComponent.h"

// ue
#include "AIController.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSecretDoorNavLinkComponent)

UTATSecretDoorNavLinkComponent::UTATSecretDoorNavLinkComponent()
{
   LinkRelativeStart = FVector(0, -80, 0);
   LinkRelativeEnd = FVector(0, 80, 0);
}

bool UTATSecretDoorNavLinkComponent::IsLinkPathfindingAllowed(const UObject* querier) const
{
   const AAIController* querierController = Cast<AAIController>(querier);
   const APawn* querierPawn = querierController ? querierController->GetPawn() : Cast<APawn>(querier);
   if (querierPawn != nullptr)
   {
      const FVector querierLocation = querierPawn->GetActorLocation();
      return FVector::Distance(querierLocation, GetStartPoint()) < UseIfWithinDistance
         || FVector::Distance(querierLocation, GetEndPoint()) < UseIfWithinDistance;
   }

   return false;
}
