// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATHubGameState.h"

// tat
#include "Player/TATDemoHubPlayerController.h"

// ue4
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATGameMode.h"
#include "Variation/DemoHubMissionMgr.h"
#include "Net/UnrealNetwork.h"
#include "Player/TATPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHubGameState)
DEFINE_LOG_CATEGORY_STATIC(LogTATHubGameState, Log, All);

/* static */
ATATHubGameState* ATATHubGameState::GetTATHubGameState(const UObject* contextObj)
{
   check(contextObj);
   return Cast<ATATHubGameState>(contextObj->GetWorld()->GetGameState());
}

/* static */
ATATHubGameState* ATATHubGameState::Get(const UObject& contextObj)
{
   return GetTATHubGameState(&contextObj);
}

ATATHubGameState::ATATHubGameState(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer
      .DoNotCreateDefaultSubobject(TEXT("MapVariationMgr"))) // we don't need MapVariationMgr in the hub, that's a game-side construct.  TODO: Make a gameplay-specific gamestate, too?
{
}

void ATATHubGameState::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      AuthoritySetMatchLobbyState(ETATMatchLobbyState::CharacterSelect);
   }
}

void ATATHubGameState::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void ATATHubGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATHubGameState, _matchLobbyState);
}

void ATATHubGameState::AuthoritySetMatchLobbyState(ETATMatchLobbyState matchLobbyState)
{
   check(HasAuthority());
   check(matchLobbyState != ETATMatchLobbyState::None);
   if (_matchLobbyState == matchLobbyState)
   {
      return;
   }

   const ETATMatchLobbyState oldMatchLobbyState = _matchLobbyState;
   UE_LOG(LogTATHubGameState, Verbose, TEXT("Setting match lobby state from %s -> %s"), *UEnum::GetValueAsString(_matchLobbyState), *UEnum::GetValueAsString(matchLobbyState));

   _matchLobbyState = matchLobbyState;
   OnMatchLobbyStateChanged.Broadcast(oldMatchLobbyState, _matchLobbyState);
}

void ATATHubGameState::_OnRep_MatchLobbyState(ETATMatchLobbyState oldState)
{
   OnMatchLobbyStateChanged.Broadcast(oldState, _matchLobbyState);
}
