// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "CharacterCustomization/TATLoadoutCaptureMgr.h"

// tat
#include "Matchmaking/Lobby/TATMatchLobbyDummyPlayer.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"

// ue5
#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLoadoutCaptureMgr)

ATATLoadoutCaptureMgr* ATATLoadoutCaptureMgr::GetLoadoutCaptureManager(const UObject* contextObject)
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
   {
      for (TActorIterator<ATATLoadoutCaptureMgr> it(world); it; ++it)
      {
         if (ATATLoadoutCaptureMgr* captureMgr = *it)
         {
            return captureMgr;
         }
      }
   }

   return nullptr;
}

void ATATLoadoutCaptureMgr::EnableCapture()
{
   bIsCaptureEnabled = true;
   OnIsCaptureEnabledChanged.Broadcast(bIsCaptureEnabled);
}

void ATATLoadoutCaptureMgr::DisableCapture()
{
   bIsCaptureEnabled = false;
   OnIsCaptureEnabledChanged.Broadcast(bIsCaptureEnabled);
}

void ATATLoadoutCaptureMgr::_OnGameStateFound(AGameStateBase* gameState)
{
   if (ATATGameState* gs = Cast<ATATGameState>(gameState))
   {
      if (_localPlayerDummy.IsValid() && !_localPlayerDummy->IsClaimed())
      {
         if (ATATPlayerState* ps = ATATPlayerState::GetLocalTATPlayerState(this))
         {
            _localPlayerDummy.Get()->AssignPlayer(ps);
         }
      }
   }
}
