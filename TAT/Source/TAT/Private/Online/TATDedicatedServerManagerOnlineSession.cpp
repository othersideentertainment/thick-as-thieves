// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATDedicatedServerManagerOnlineSession.h"

// tat
#include "TATGameInstance.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATTravelMgr.h"
#include "Online/TATGameState.h"
#include "Quests/TATContractPrioritizer.h"
#include "Settings/TATMatchSettings.h"
#include "Player/TATPlayerController.h"

// ose
#include "OSEDedicatedServerSubsystem.h"

// ue
#include "Online.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDedicatedServerManagerOnlineSession)

DEFINE_LOG_CATEGORY_STATIC(LogTATDedicatedServerManagerOnlineSession, Log, All);

// static
UTATDedicatedServerManagerOnlineSession& UTATDedicatedServerManagerOnlineSession::GetChecked(UObject* worldContextObject)
{
   check(worldContextObject != nullptr);
   UGameInstance* gameInstance = worldContextObject->GetWorld()->GetGameInstance();
   check(gameInstance != nullptr);
   UOSEDedicatedServerSubsystem* serverSubsystem = gameInstance->GetSubsystem<UOSEDedicatedServerSubsystem>();
   check(serverSubsystem != nullptr);
   return *CastChecked<UTATDedicatedServerManagerOnlineSession>(serverSubsystem->GetDedicatedServerMgr());
}

// static
UTATDedicatedServerManagerOnlineSession* UTATDedicatedServerManagerOnlineSession::Get(UObject* worldContextObject)
{
   if (worldContextObject != nullptr)
   {
      if (UGameInstance* gameInstance = worldContextObject->GetWorld()->GetGameInstance())
      {
         if (UOSEDedicatedServerSubsystem* serverSubsystem = gameInstance->GetSubsystem<UOSEDedicatedServerSubsystem>())
         {
            return Cast<UTATDedicatedServerManagerOnlineSession>(serverSubsystem->GetDedicatedServerMgr());
         }
      }
   }
   return nullptr;
}

void UTATDedicatedServerManagerOnlineSession::Init(UGameInstance* gameInstance)
{
   Super::Init(gameInstance);
   UE_LOG(LogTATDedicatedServerManagerOnlineSession, Verbose, TEXT("Init(gameInstance='%s')"), *GetNameSafe(gameInstance));
}

void UTATDedicatedServerManagerOnlineSession::Shutdown()
{
   Super::Shutdown();
   UE_LOG(LogTATDedicatedServerManagerOnlineSession, Verbose, TEXT("Shutdown"));

   IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld());
   if (sessionInterface.IsValid())
   {
      const bool success = sessionInterface->DestroySession(NAME_GameSession);
      if (!success)
      {
         UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("sessionInterface->DestroySession() failed"));
      }
   }
}

void UTATDedicatedServerManagerOnlineSession::ServerReadyToAcceptPlayers()
{
   Super::ServerReadyToAcceptPlayers();

   IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld());
   if (!sessionInterface.IsValid())
   {
      UE_LOG(LogTATDedicatedServerManagerOnlineSession, Fatal, TEXT("Failed to get session interface"));
      return;
   }

   FOnlineSessionSettings sessionSettings;
   sessionSettings.NumPublicConnections = _GetMaxPlayerCount();
   sessionSettings.bShouldAdvertise = true;
   sessionSettings.bAllowJoinInProgress = true;
   sessionSettings.bIsDedicated = true;

   FParse::Bool(FCommandLine::Get(), TEXT("-LanMatch="), sessionSettings.bIsLANMatch);

   // SteamOSS doesn't handle custom search parameters (see FOnlineAsyncTaskSteamFindServerBase::CreateQuery)
   // nor querying for sessions by session identifier, so we use the "map name" parameter to uniquely identify
   // the server session; see also ATATPlayerController::ClientJoinOnlineSession_Implementation
   sessionSettings.Set(SETTING_MAPNAME, FApp::GetSessionId().ToString(EGuidFormats::Base36Encoded), EOnlineDataAdvertisementType::ViaOnlineService);

   const TSharedRef<FDelegateHandle> createHandle = MakeShared<FDelegateHandle>();
   *createHandle = sessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
      FOnCreateSessionCompleteDelegate::CreateWeakLambda(this, [this, sessionInterface, createHandle](FName, bool bSuccess)
      {
         if (!bSuccess)
         {
            UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("Failed to create session"))
         }

         sessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(*createHandle);
      }));

   if (!sessionInterface->CreateSession(0, NAME_GameSession, sessionSettings))
   {
      UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("Failed to create session"));

      sessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(*createHandle);
   }
}

void UTATDedicatedServerManagerOnlineSession::ServerEndMatch()
{
   {
      // Wait a few seconds, then run match-end cleanup (includes exiting the process if this is a dedicated server)
      GetWorld()->GetTimerManager().SetTimer(
         _matchEndCleanupTimer,
         this,
         &ThisClass::_OnMatchEndCleanup,
         UTATProjectSettings::Get().CleanupDelayAfterMatchEndSeconds
      );
   }
}

void UTATDedicatedServerManagerOnlineSession::PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   Super::PreLogin(options, address, uniqueId, errorMessage);
   UE_LOG(LogTATDedicatedServerManagerOnlineSession, Verbose, TEXT("PreLogin(options='%s', address='%s', uniqueId='%s', errorMessage='%s')"), *options, *address, *uniqueId.ToString(), *errorMessage);
}

void UTATDedicatedServerManagerOnlineSession::Login(const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   Super::Login(options, uniqueId, errorMessage);
   UE_LOG(LogTATDedicatedServerManagerOnlineSession, Log, TEXT("Login(options='%s', uniqueId='%s', errorMessage='%s')"), *options, *uniqueId.ToString(), *errorMessage);
   
   IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld());
   if (sessionInterface.IsValid())
   {
         if (FUniqueNetIdPtr playerId = uniqueId.GetV1())
         {
            // NB. Registering players to a session is what causes the player count in the server browser to go up
            constexpr bool wasInvited = false;
            const bool registerSuccess = sessionInterface->RegisterPlayer(NAME_GameSession, *uniqueId.GetV1(), wasInvited);
            if (!registerSuccess)
            {
               UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("RegisterPlayer(playerId='%s') failed"), *playerId->ToString());
            }
         }
         else
         {
            UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("Login: Invalid player id"));
         }
   }
   else
   {
      UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("Logout: session interface was invalid"));
   }
}

void UTATDedicatedServerManagerOnlineSession::PostLogin(AController* newPlayer)
{
   Super::PostLogin(newPlayer);
}

void UTATDedicatedServerManagerOnlineSession::Logout(AController* exitingController)
{
   Super::Logout(exitingController);
   UE_LOG(LogTATDedicatedServerManagerOnlineSession, Log, TEXT("Logout(exitingController='%s')"), *GetNameSafe(exitingController));

   IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld());
   if (sessionInterface.IsValid())
   {
      FUniqueNetIdPtr playerId = nullptr;
      if (exitingController)
      {
         if (APlayerState* ps = exitingController->GetPlayerState<APlayerState>())
         {
            playerId = ps->GetUniqueId().GetV1();
         }
      }

      if (playerId.IsValid())
      {
         // NB. Unregistering the player from the session causes the player count in the server browser to go down
         const bool registerSuccess = sessionInterface->UnregisterPlayer(NAME_GameSession, *playerId);
         if (!registerSuccess)
         {
            UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("UnregisterPlayer(playerId='%s') failed"), *playerId->ToString());
         }
      }
      else
      {
         UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("Logout: Invalid player id"));
      }
   }
   else
   {
      UE_LOG(LogTATDedicatedServerManagerOnlineSession, Error, TEXT("Logout: session interface was invalid"));
   }
}

void UTATDedicatedServerManagerOnlineSession::Tick(float deltaTime)
{
   Super::Tick(deltaTime);
}

int32 UTATDedicatedServerManagerOnlineSession::_GetMaxPlayerCount() const
{
   UTATGameInstance* tatGameInstance = Cast<UTATGameInstance>(_gameInstance);
   return (tatGameInstance != nullptr) ? tatGameInstance->GetMaxNumPlayersForSession() : 1;
}

int32 UTATDedicatedServerManagerOnlineSession::_GetNumConnectedPlayers() const
{
   if (UWorld* world = GetWorld())
   {
      if (AGameModeBase* gameMode = world->GetAuthGameMode())
      {
         return gameMode->GetNumPlayers();
      }
   }
   return 0;
}

void UTATDedicatedServerManagerOnlineSession::_OnMatchEndCleanup()
{
   _matchEndCleanupTimer.Invalidate();

   // Allow ignoring all auto-exit behavior if this command line flag is used. Handy for debugging.
   if (FParse::Param(FCommandLine::Get(), TEXT("-ForcePreventExitAtMatchEnd")))
   {
      return;
   }

   // Only respect the config option if this is a dedicated server and not the editor
   const bool configAutoExit = UTATProjectSettings::Get().AutoExitDedicatedServerProcess && !GetWorld()->IsEditorWorld();

   // If we get the auto-exit command line flag, assume the user knows what they want and ignore the build type and net mode.
   // This parses a bool value to make it more ergonomic to use as a variable in the server manager.
   bool commandLineExitAtMatchEnd = false;
   FParse::Bool(FCommandLine::Get(), TEXT("-ExitAtMatchEnd="), commandLineExitAtMatchEnd);

   if (configAutoExit || commandLineExitAtMatchEnd)
   {
      UE_LOG(LogTATDedicatedServerManagerOnlineSession, Log, TEXT("Completed match cleanup; exiting dedicated server"));
      static constexpr bool forceExit = false;
      static constexpr uint8 returnCode = 0;
      static constexpr const TCHAR* callSite = TEXT("ATATInGameGameMode::_OnMatchEndCleanup()");
      FPlatformMisc::RequestExitWithStatus(forceExit, returnCode, callSite);
   }
}
