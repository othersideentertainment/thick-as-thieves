// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATGameSession.h"

// tat
#include "GameFramework/TATGameMode.h"
#include "Online/TATHubGameMode.h"
#include "Player/TATPlayerState.h"
#include "TATGameInstance.h"

// ue
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameSession)

bool ATATGameSession::AtCapacity(bool bSpectator)
{
   if (bSpectator)
   {
      return Super::AtCapacity(bSpectator);
   }

   // The total number of logged in and currently-logging-in players
   int32 numCurrentAndPendingPlayers = 0;

   if (AGameModeBase* gameModeBase = GetWorld()->GetAuthGameMode())
   {
      numCurrentAndPendingPlayers = gameModeBase->GetNumPlayers();

      //TODO: move this functionality into a shared base class or find some other mechanism to avoid the duplicated functionality
      if (ATATGameMode* tatGameMode = Cast<ATATGameMode>(gameModeBase))
      {
         numCurrentAndPendingPlayers += tatGameMode->GetNumPlayersPendingLogin();
      }
      else if (ATATHubGameMode* hubGameMode = Cast<ATATHubGameMode>(gameModeBase))
      {
         numCurrentAndPendingPlayers += hubGameMode->GetNumPlayersPendingLogin();
      }
   }

   // NB. Unfortunately, this doesn't account for the cvar net.MaxPlayersOverride, because that cvar
   // is only accessible from GameSession.cpp and it doesn't expose the value publicly.
   return MaxPlayers > 0 && numCurrentAndPendingPlayers >= MaxPlayers;
}

void ATATGameSession::RegisterPlayer(APlayerController* newPlayer, const FUniqueNetIdRepl& uniqueId, bool wasFromInvite)
{
   Super::RegisterPlayer(newPlayer, uniqueId, wasFromInvite);

   if (newPlayer != nullptr)
   {
      newPlayer->PlayerState->SetPlayerId(_GetPlayerIdFor(uniqueId));

      if (ATATPlayerState* tatPlayerState = Cast<ATATPlayerState>(newPlayer->PlayerState))
      {
         // Notify that it should initialize the quest after setting PlayerId
         tatPlayerState->AuthorityInitializeQuest();
      }
   }
}

int32 ATATGameSession::_GetPlayerIdFor(const FUniqueNetIdRepl& uniqueId)
{
   const UTATGameInstance* gameInstance = GetWorld()->GetGameInstanceChecked<UTATGameInstance>();

   // if a party member, use index-in-party + 1
   // Q: Should it skip this outside of match levels? It is mostly benign, but could cause collisions if the party on the instance changed on the fly (it is currently set immediately before travel, which is fine)
   const int32 idFromParty = gameInstance->TryPredictPlayerIdForPartyMember(uniqueId);
   if (idFromParty != INDEX_NONE)
   {
      return idFromParty;
   }

   // otherwise auto-increment starting after the party
   if (_nextPlayerId < 0)
   {
      _nextPlayerId = gameInstance->GetPartySize() + 1;
   }

   const int32 result = _nextPlayerId;
   _nextPlayerId++;
   return result;
}
