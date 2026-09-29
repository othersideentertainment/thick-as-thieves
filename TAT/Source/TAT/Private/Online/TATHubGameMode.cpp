// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATHubGameMode.h"

// tat
#include "Developer/TATEditorSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Online/TATGameSession.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerController.h"
#include "Settings/TATMatchSettingsPropertyDef.h"
#include "TATGameInstance.h"

// ose dedicated server
#include "ServerManager/OSEDedicatedServerManagerBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHubGameMode)

DEFINE_LOG_CATEGORY_STATIC(LogTATHubGameMode, Log, All);

ATATHubGameMode::ATATHubGameMode()
   : Super()
{
}

void ATATHubGameMode::StartPlay()
{
   Super::StartPlay();
}

void ATATHubGameMode::BeginPlay()
{
   Super::BeginPlay();
   UE_LOG(LogTATHubGameMode, Log, TEXT("TAT Hub Game Mode Begin Play =============================================="));

   ATATWorldSettings& worldSettings = ATATWorldSettings::Get(GetWorld());

#if OSE_CHEATS_ENABLED
   // If we have any default match settings to apply, do it now
   UTATGameInstance& gameInstance = UTATGameInstance::Get(this);
   UTATEditorSettings::Get().ApplyDefaultMatchSettings(gameInstance.GetMatchSettings(), FTATMatchSettingsQueryContext::MakeFromWorldContext(this));
   gameInstance.AuthorityNotifyCheatUpdatedMatchSettings();
#endif

   // activate a dedicated server once we've loaded into the hub
   if (IsNetMode(NM_DedicatedServer))
   {
      UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
      serverMgr.ServerReadyToAcceptPlayers();
   }
}

bool ATATHubGameMode::AllowCheats(APlayerController* pc)
{
#if UE_BUILD_DEVELOPMENT
   // dev builds allow cheating across the board
   return true;
#else
   return GIsEditor || GetNetMode() == NM_Standalone;
#endif
}

void ATATHubGameMode::PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   if (errorMessage.IsEmpty())
   {
      // accept/reject players by setting the error string
      if (IsNetMode(NM_DedicatedServer))
      {
         UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
         serverMgr.PreLogin(options, address, uniqueId, errorMessage);
      }
   }

   // Call ApproveLogin directly rather than relying on the parent since it requires that the client and server OSS match
   // and will error out with incompatible_unique_net_id
   // NB. Super::PreLogin handles server capacity checks
   if (errorMessage.IsEmpty())
   {
      errorMessage = GameSession->ApproveLogin(options);
      FGameModeEvents::GameModePreLoginEvent.Broadcast(this, uniqueId, errorMessage);
   }

   if (!errorMessage.IsEmpty())
   {
      UE_LOG(LogTATHubGameMode, Error, TEXT("PreLogin() Error: %s"), *errorMessage);
      return;
   }
   
   // Track this player's unique id until they're fully logged in (eg. have an assigned player controller) so that we can count the number of pending logins
   _pendingPlayerLogins.AddUnique(uniqueId);

   // If the player hasn't completed the login within a few seconds, assume they've disconnected or otherwise left the login process
   const float playerLoginTimeoutSeconds = 5.0f;
   FTimerHandle handle;
   TWeakObjectPtr<ATATHubGameMode> weakThis = this;
   GetWorldTimerManager().SetTimer(handle, [weakThis, uniqueId]()
      {
         if (ATATHubGameMode* self = weakThis.Get())
         {
            const int32 playerTimedOut = self->_pendingPlayerLogins.RemoveSingleSwap(uniqueId);
            if (playerTimedOut > 0)
            {
               UE_LOG(LogTATHubGameMode, Warning, TEXT("Player login timed out (%s)"), *uniqueId.ToString());
            }
         }
      }, playerLoginTimeoutSeconds, false);
}

APlayerController* ATATHubGameMode::Login(UPlayer* newPlayer, ENetRole inRemoteRole, const FString& portal, const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   // Player has logged in successfully, so remove them from the list of pending players to free up a slot for them.
   // This *MUST* happen before calling Super::Login, because that will do a server capacity check.
   // See also: ATATGameSession::AtCapacity()
   _pendingPlayerLogins.RemoveSingleSwap(uniqueId);

   APlayerController* pc = Super::Login(newPlayer, inRemoteRole, portal, options, uniqueId, errorMessage);

   if (pc && IsNetMode(NM_DedicatedServer))
   {
      UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
      serverMgr.Login(options, uniqueId, errorMessage);
   }

   return pc;
}

void ATATHubGameMode::PostLogin(APlayerController* newPlayer)
{
   Super::PostLogin(newPlayer);

   if (IsNetMode(NM_DedicatedServer))
   {
      UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
      serverMgr.PostLogin(newPlayer);
   }
}

void ATATHubGameMode::Logout(AController* exitingController)
{
   Super::Logout(exitingController);

   if (IsNetMode(NM_DedicatedServer))
   {
      // The subsystem may have been torn down already when ending PIE (as of 5.4)
      if (UOSEDedicatedServerManagerBase* serverMgr = UOSEDedicatedServerManagerBase::TryGet(*this))
      {
         serverMgr->Logout(exitingController);
      }
   }
}

void ATATHubGameMode::GetSeamlessTravelActorList(bool toTransition, TArray<AActor*>& actorList)
{
   Super::GetSeamlessTravelActorList(toTransition, actorList);

   // server can add actors to seamless travel here
   for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
   {
      APlayerController* pc = it->Get();
      actorList.AddUnique(pc);
      if (APawn* pawn = pc->GetPawn())
      {
         actorList.AddUnique(pawn);
      }
   }
}

void ATATHubGameMode::HandleSeamlessTravelPlayer(AController*& pc)
{
   Super::HandleSeamlessTravelPlayer(pc);
}
