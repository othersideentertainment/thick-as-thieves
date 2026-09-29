// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Player/TATSpectatorPawn.h"

// ue
#include "EnhancedInputComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

// tat
#include "GameFramework/TATMatchPersistenceGameInstanceSubsystem.h"
#include "Player/TATPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpectatorPawn)

DEFINE_LOG_CATEGORY_STATIC(LogTATSpectatorPawn, Log, All)

ATATSpectatorPawn::ATATSpectatorPawn(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   bReplicates = false;
}

void ATATSpectatorPawn::PossessedBy(AController* newController)
{
   const UGameInstance* gameInstance = GetGameInstance();
   check(gameInstance);
   _persistenceGameInstanceSubsystem = gameInstance->GetSubsystem<UTATMatchPersistenceGameInstanceSubsystem>();
   check(_persistenceGameInstanceSubsystem);
   
   Super::PossessedBy(newController);
   
   _OnCycleNextPlayer();
}

void ATATSpectatorPawn::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
   Super::SetupPlayerInputComponent(playerInputComponent);
   if (UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(playerInputComponent))
   {
      enhancedInputComponent->BindAction(_cycleNextPlayerAction, ETriggerEvent::Completed, this, &ThisClass::_OnCycleNextPlayer);
      enhancedInputComponent->BindAction(_cyclePrevPlayerAction, ETriggerEvent::Completed, this, &ThisClass::_OnCyclePrevPlayer);
      enhancedInputComponent->BindAction(_finishEscapingAction, ETriggerEvent::Completed, this, &ThisClass::_OnFinishEscaping);
   }
}

bool ATATSpectatorPawn::_IsInEscapedState() const
{
   if(_persistenceGameInstanceSubsystem == nullptr || _persistenceGameInstanceSubsystem->HasDataToPresent() == false)
   {
      return false;
   }
   return true;
}

void ATATSpectatorPawn::_OnCyclePrevPlayer()
{
   _CyclePlayer(-1);
}

void ATATSpectatorPawn::_OnCycleNextPlayer()
{
   _CyclePlayer(1);
}

void ATATSpectatorPawn::_OnFinishEscaping()
{
   if (_IsInEscapedState() == false)
      return;

   if(GetNetMode() == NM_ListenServer && HasAuthority())
   {
      // if we're the server host, we can't fully escape otherwise it'd removal all other players
      return;
   }
   if(ATATPlayerController* playerController = GetController<ATATPlayerController>())
   {
      if (GetNetMode() != NM_Standalone)
      {
         UE_LOG(LogTATSpectatorPawn, Warning, TEXT("Leaving match early, will not have ranking information for other players"));
      }

      // NB: Since we're leaving early, we won't have other player stats on the rack-up screen
      // We could potentially try to track other players as they escape and get some rankings,
      // but that complicates the code and may be confusing
      // For now, players need to wait until the server ends the match to get other players' ranking information
      playerController->ClientFinishEscaping();
   }
}

void ATATSpectatorPawn::_CyclePlayer(const int direction)
{
   if(ATATPlayerController* playerController = GetController<ATATPlayerController>())
   {
      if(APlayerState* nextViewableSpectatorPlayerState = _GetNextPlayerToSpectate(direction))
      {
         playerController->SetPlayerCameraMode(EPlayerCameraMode::Default);
         if(_followedPlayer)
         {
            if(APawn* followedPlayerPawn = _followedPlayer->GetPawn())
            {
               RemoveTickPrerequisiteActor(followedPlayerPawn);
            }
         }
         _followedPlayer = nextViewableSpectatorPlayerState;
         AddTickPrerequisiteActor(nextViewableSpectatorPlayerState->GetPawn());
         playerController->SetPlayerCameraMode(EPlayerCameraMode::ThirdPerson);
         OnCycleFollowedPlayer.Broadcast(_followedPlayer);
      }
   }
}

void ATATSpectatorPawn::Tick(const float deltaSeconds)
{
   Super::Tick(deltaSeconds);
   if(IsValid(_followedPlayer) == false)
   {
      return;
   }
   if(_followedPlayer->IsInactive() || _followedPlayer->IsSpectator())
   {
      _CyclePlayer(1);
      return;
   }
   if(const APawn* followedPlayerPawn = _followedPlayer->GetPawn())
   {
      SetActorLocation(followedPlayerPawn->GetActorLocation(), false, nullptr, ETeleportType::None);
   }
}

// Very similar to APlayerController::GetNextViewablePlayer, however it doesn't use the game mode
// uses the local _followedPlayer variable to cycle through all characters
APlayerState* ATATSpectatorPawn::_GetNextPlayerToSpectate(const int direction) const
{
   const UWorld* world = GetWorld();
   AGameStateBase* gameState = world->GetGameState();

   // Can't continue unless we have the gameState
   if (gameState == nullptr)
   {
      return nullptr;
   }

   APlayerState* nextPlayerState = _followedPlayer;
	
   // If we don't have a NextPlayerState, use our own.
   // This will allow us to attempt to find another player to view or,
   // if all else fails, makes sure we have a player state set for next time.
   int32 nextIndex = (nextPlayerState
                         ? gameState->PlayerArray.Find(nextPlayerState)
                         : gameState->PlayerArray.Find(
                            GetController<APlayerController>()->GetPlayerState<APlayerState>()));

   //Check that NextIndex is a valid index, as Find() may return INDEX_NONE
   if (!gameState->PlayerArray.IsValidIndex(nextIndex))
   {
      return nullptr;
   }

   // Cycle through the player states until we find a valid one.
   for (int32 i = 0; i < gameState->PlayerArray.Num(); ++i)
   {
      nextIndex = ((nextIndex == 0) && (direction < 0))
                     ? (gameState->PlayerArray.Num() - 1)
                     : ((nextIndex == (gameState->PlayerArray.Num() - 1)) && (direction > 0))
                     ? 0
                     : nextIndex += direction;
      nextPlayerState = gameState->PlayerArray[nextIndex];

      // Make sure we're not trying to view our own player state.
      if (nextPlayerState != GetPlayerState())
      {
         if (nextPlayerState->GetPawn())
         {
            break;
         }
      }
   }
   return nextPlayerState;
}
