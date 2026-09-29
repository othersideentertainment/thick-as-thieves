// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Player/TATDemoHubPlayerController.h"

// tat
#include "Online/TATHubGameState.h"
#include "UI/TATScreenMgr.h"
#include "UI/TATScreenWidget.h"
#include "Variation/DemoHubMissionMgr.h"

// ue
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDemoHubPlayerController)
DEFINE_LOG_CATEGORY_STATIC(LogTATDemoHubPlayerController, Log, All);


void ATATDemoHubPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> newHUDClass)
{
   Super::ClientSetHUD_Implementation(newHUDClass);

   UWorld* world = GetWorld();
   check(world);
   if (AGameStateBase* gs = world->GetGameState())
   {
      _OnGameStateSet(gs);
   }
   else
   {
      world->GameStateSetEvent.AddUObject(this, &ATATDemoHubPlayerController::_OnGameStateSet);
   }
}

void ATATDemoHubPlayerController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UWorld* world = GetWorld())
   {
      world->GameStateSetEvent.RemoveAll(this);
      if (ATATHubGameState* tatHubGs = world->GetGameState<ATATHubGameState>())
      {
         tatHubGs->OnMatchLobbyStateChanged.RemoveAll(this);
      }
   }

   Super::EndPlay(endPlayReason);
}

void ATATDemoHubPlayerController::SetDestinationMap(const TSoftObjectPtr<UWorld>& newMap, FSoftClassPath mapPrefix)
{
   _ServerSetDestinationMap(newMap, mapPrefix);
}

void ATATDemoHubPlayerController::_OnMatchLobbyStateChanged(ETATMatchLobbyState oldMatchLobbyState, ETATMatchLobbyState newMatchLobbyState)
{
   UE_LOG(LogTATDemoHubPlayerController, Verbose, TEXT("Lobby switching from state %s -> %s"), *UEnum::GetValueAsString(oldMatchLobbyState), *UEnum::GetValueAsString(newMatchLobbyState));
   check(newMatchLobbyState != ETATMatchLobbyState::None);
   if (IsNetMode(NM_DedicatedServer))
   {
      return;
   }
   check(IsLocalPlayerController());

   UTATScreenMgr* screenMgr = GetTATScreenMgr();
   check(screenMgr);

   // Remove screen pertaining to last state, if present
   if (oldMatchLobbyState != ETATMatchLobbyState::None)
   {
      TSubclassOf<UTATScreenWidget> oldScreenClass = _GetScreenForState(oldMatchLobbyState);
      UTATScreenWidget* topScreen = screenMgr->GetTopScreen();
      if (topScreen && topScreen->IsA(oldScreenClass))
      {
         UE_LOG(LogTATDemoHubPlayerController, Verbose, TEXT("Removing screen %s from previous state (%s) "), *topScreen->GetName(), *UEnum::GetValueAsString(oldMatchLobbyState));
         screenMgr->RemoveScreen(topScreen);
      }
   }
   
   // Add new screen
   const TSubclassOf<UTATScreenWidget> screenClass = _GetScreenForState(newMatchLobbyState);
   if (IsValid(screenClass))
   {
      UE_LOG(LogTATDemoHubPlayerController, Verbose, TEXT("Creating screen for new state (%s) "), *UEnum::GetValueAsString(newMatchLobbyState));
      UTATScreenWidget* screenWidget = CreateWidget<UTATScreenWidget>(this, screenClass);
      check(IsValid(screenWidget));
      screenMgr->AddScreen(screenWidget);
   }
   else
   {
      UE_LOG(LogTATDemoHubPlayerController, Error, TEXT("_GetScreenForState() returned invalid screen class for state %s!"), *UEnum::GetValueAsString(newMatchLobbyState));
   }
}

void ATATDemoHubPlayerController::_OnGameStateSet(AGameStateBase* gameState)
{
   // Bind to match lobby state changes
   ATATHubGameState* tatHubGs = CastChecked<ATATHubGameState>(gameState);
   tatHubGs->OnMatchLobbyStateChanged.AddUObject(this, &ATATDemoHubPlayerController::_OnMatchLobbyStateChanged);

   // Unbind from world's GameStateSetEvent
   GetWorld()->GameStateSetEvent.RemoveAll(this);

   // Setup from current match lobby state (if assigned)
   const ETATMatchLobbyState matchLobbyState = tatHubGs->GetMatchLobbyState();
   if (matchLobbyState != ETATMatchLobbyState::None)
   {
      _OnMatchLobbyStateChanged(ETATMatchLobbyState::None, matchLobbyState);
   }
}

TSubclassOf<UTATScreenWidget> ATATDemoHubPlayerController::_GetScreenForState(ETATMatchLobbyState matchLobbyState) const
{
   switch (matchLobbyState)
   {
   case ETATMatchLobbyState::SetupMatch:
      return SetupMatchScreen;
      
   case ETATMatchLobbyState::CharacterSelect:
      return CharacterSelectScreen;

   default:
      checkNoEntry();
   }
   return TSubclassOf<UTATScreenWidget>();
}

void ATATDemoHubPlayerController::_ServerSetDestinationMap_Implementation(const TSoftObjectPtr<UWorld>& newMap, const FSoftClassPath& gameMode)
{
   if (auto* missionManager = Cast<ATATDemoHubMissionMgr>(UGameplayStatics::GetActorOfClass(GetWorld(), ATATDemoHubMissionMgr::StaticClass())))
   {
      missionManager->AuthoritySetSelectedMap(newMap, gameMode);
   }
}
