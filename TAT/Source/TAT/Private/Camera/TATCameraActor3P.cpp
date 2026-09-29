// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/TATCameraActor3P.h"

// TAT
#include "Player/TATSpectatorPawn.h"

// UE
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCameraActor3P)

const AActor* ATATCameraActor3P::GetTargetActorForCamera() const
{
   const AActor* superReturnValue = Super::GetTargetActorForCamera();
   if (const APlayerController* playerController = Cast<APlayerController>(GetOwner()))
   {
      if (const ATATSpectatorPawn* spectatorPawn = Cast<ATATSpectatorPawn>(playerController->GetSpectatorPawn()))
      {
         if(const APlayerState* followedPlayerState = spectatorPawn->GetFollowedPlayerState())
         {
            if(const APawn* followedPawn = followedPlayerState->GetPawn())
            {
               return followedPawn;
            }
         }
      }
   }
   return superReturnValue;
}
