// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ServerManager/OSEDedicatedServerManagerGameLift.h"

// ose dedicated server gamelift
#include "OSEDedicatedServerGameliftSettings.h"

// ue4
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

// gamelift
#include <aws/gamelift/server/model/GameSession.h>
#include <aws/gamelift/server/model/GameSessionStatus.h>

// general flow taken from AWS docs:
// @ https://github.com/awsdocs/amazon-gamelift-developer-guide/blob/master/doc_source/gamelift-sdk-server-api.md
// @ https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-server-sdk-cpp-ref-actions.html#integration-server-sdk-cpp-ref-processending

namespace GameliftSDKUtl
{
   // shut down the server if no players land on it in this timeframe, otherwise the server will just stay up forever
   static const float kIdleGameSessionDuration = 60.0f * 3.0f; // 3m
   static const float kRemoveFromSessionAfterPlayerNotSeenFor = 60.0f * 3.0f; // 3m

   template <typename R, typename E>
   void LogOutcome(const FString& operation, const TGameLiftOutcome<R, E>& outcome)
   {
      const bool success = outcome.IsSuccess();
      if (success)
      {
         UE_LOG(LogOSEDedicatedServer, Log, TEXT("Gamelift: %s SUCCESS"), *operation);
      }
      else
      {
         FGameLiftError error = outcome.GetError();
         UE_LOG(LogOSEDedicatedServer, Error, TEXT("Gamelift: %s FAILED"), *operation);
         UE_LOG(LogOSEDedicatedServer, Error, TEXT("  - m_errorName: \"%s\""), *error.m_errorName);
         UE_LOG(LogOSEDedicatedServer, Error, TEXT("  - m_errorMessage: \"%s\""), *error.m_errorMessage);
      }
   }
}

void UOSEDedicatedServerManagerGameLift::Init(UGameInstance* gameInstance)
{
   Super::Init(gameInstance);
   _GameliftProcessReady();
}

void UOSEDedicatedServerManagerGameLift::Shutdown()
{
   Super::Shutdown();

   UE_LOG(LogOSEDedicatedServer, Log, TEXT("Process Ending because we received Shutdown() of the dedicated server manager."));
   _GameliftProcessEnding();
}

void UOSEDedicatedServerManagerGameLift::ServerReadyToAcceptPlayers()
{
   Super::ServerReadyToAcceptPlayers();

   // we should have started the gamelift sdk by now
   check(_gameLiftSdkModule);

   // we can mark ourselves as ready for players in any state, and it will be consumed by
   // Tick() - EOSEGameliftServerState::WaitingForServerToAcceptPlayers at some point
   _isReadyForPlayers = true;
}

void UOSEDedicatedServerManagerGameLift::PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   Super::PreLogin(options, address, uniqueId, errorMessage);

   const UOSEDedicatedServerGameliftSettings& settings = UOSEDedicatedServerGameliftSettings::Get();
   FString playerSessionId = UGameplayStatics::ParseOption(options, settings.PlayerSessionIdOptionsKey);
   FString gameSessionId = UGameplayStatics::ParseOption(options, settings.GameSessionIdOptionsKey);

   if (!uniqueId.IsValid())
   {
      errorMessage = TEXT("Invalid unique net id, will not allow this player on the server");
      return;
   }

   if (playerSessionId.IsEmpty())
   {
      errorMessage = TEXT("Invalid player session id, will not allow this player on the server");
      return;
   }

   if (gameSessionId.IsEmpty())
   {
      errorMessage = TEXT("Invalid game session id, will not allow this player on the server");
      return;
   }

   // ensure that this player is supposed to be joining this specific game server instance
   if (errorMessage.IsEmpty())
   {
      if (gameSessionId != _gameLiftState.GameSessionArn)
      {
         errorMessage = FString::Printf(TEXT("Player \"%s\" is trying to join game session \"%s\" but this server instance is running game session \"%s\"!"),
            *playerSessionId, *gameSessionId, *_gameLiftState.GameSessionArn);
         return;
      }

      // only accept the player session once (we get multiple prelogin/login/logout calls as we traverse maps)
      if (!_playerInfo.Contains(uniqueId))
      {
         if (!_GameliftAcceptPlayerSession(playerSessionId))
         {
            errorMessage = FString::Printf(TEXT("Player session \"%s\" is trying to join game session \"%s\" but this server instance rejected the player session!"), *playerSessionId, *gameSessionId);
            return;
         }
      }

      // TODO: Ensure that this player is in the current matchmaking ticket?  Will probably want to do that if/when I do backfill
   }
}

void UOSEDedicatedServerManagerGameLift::Login(const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   Super::Login(options, uniqueId, errorMessage);

   // we checked these on PreLogin so we can assert that they're real now

   check(uniqueId.IsValid());

   if (!_playerInfo.Contains(uniqueId))
   {
      FOSEDedicatedServerGameliftPlayerInfo& playerInfo = _playerInfo.FindOrAdd(uniqueId);

      // store player session id
      const UOSEDedicatedServerGameliftSettings& settings = UOSEDedicatedServerGameliftSettings::Get();
      FString playerSessionId = UGameplayStatics::ParseOption(options, settings.PlayerSessionIdOptionsKey);
      check(!playerSessionId.IsEmpty());
      playerInfo.PlayerSessionId = playerSessionId;
   }
}

void UOSEDedicatedServerManagerGameLift::Logout(AController* exitingController)
{
   Super::Logout(exitingController);

   check(exitingController);
   
   // NOTE: This is called when we traverse maps, too, so we can't do our remove sessions here
}

void UOSEDedicatedServerManagerGameLift::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   switch(_state)
   {
   case EOSEGameliftServerState::WaitingForServerToAcceptPlayers:
      {
         if (_isReadyForPlayers)
         {
            // activate the game session, allowing players to start joining the server
            _GameliftActivateGameSession();
         }
      }
      break;
   case EOSEGameliftServerState::InGameSession:
      {
         if (_playerInfo.Num() == 0)
         {         
            _timeSinceGameSessionActivation += deltaTime;
            if (_timeSinceGameSessionActivation > GameliftSDKUtl::kIdleGameSessionDuration)
            {
               // terminate this session, no one has been in it for a few minutes
               UE_LOG(LogOSEDedicatedServer, Log, TEXT("Process Ending because there are no players on the server"));
               _GameliftProcessEnding();
            }
         }
         else
         {
            // back to zero
            _timeSinceGameSessionActivation = 0.0f;
         }
      }
      break;
   }

   _TickTimeSincePlayerSeen(deltaTime);
}

void UOSEDedicatedServerManagerGameLift::_UpdateServerStateFromGameSession(Aws::GameLift::Server::Model::GameSession gameSession)
{
   _gameLiftState.FillFrom(gameSession);
   _gameLiftState.DumpLog();
}

void UOSEDedicatedServerManagerGameLift::_GameliftProcessReady()
{
   check(!_gameLiftSdkModule);

   _gameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));

   // InitSDK establishes a local connection with GameLift's agent to enable communication.
   FGameLiftGenericOutcome initSDKOutcome = _gameLiftSdkModule->InitSDK();
   GameliftSDKUtl::LogOutcome(TEXT("InitSDK"), initSDKOutcome);

   // Respond to new game session activation request. GameLift sends activation request 
   // to the game server along with a game session object containing game properties 
   // and other settings. Once the game server is ready to receive player connections, 
   // invoke GameLiftServerAPI.ActivateGameSession()
   auto onGameSession = [&](Aws::GameLift::Server::Model::GameSession gameSession)
   {
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("onGameSession callback"));

      // we should be waiting for our game session
      check(_state == EOSEGameliftServerState::WaitingForGameSession);

      // update the game session
      _UpdateServerStateFromGameSession(gameSession);

      // next state is waiting for the server to be ready to accept players onto it
      _state = EOSEGameliftServerState::WaitingForServerToAcceptPlayers;
   };

   auto onUpdateGameSession = [&](Aws::GameLift::Server::Model::UpdateGameSession updateGameSession)
   {
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("onUpdateGameSession callback"));

      UE_LOG(LogOSEDedicatedServer, Log, TEXT("Reason: %s"), *Aws::GameLift::Server::Model::UpdateReasonMapper::GetNameForUpdateReason(updateGameSession.GetUpdateReason()));
      
      FString backfillTicketId = updateGameSession.GetBackfillTicketId();
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("Backfill ticket id: %s"), *backfillTicketId);
      if (_gameLiftState.BackfillTicketId != backfillTicketId)
      {
         UE_LOG(LogOSEDedicatedServer, Error, TEXT("Backfill ticket id %s is not the same as the ticket we think we're using (%s)"), *backfillTicketId, *_gameLiftState.BackfillTicketId);
      }

      switch (updateGameSession.GetUpdateReason())
      {
      case Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED:
         {
            // update the game session, it's changed
            //_UpdateServerStateFromGameSession(updateGameSession.GetGameSession());
         }
         break;
      case Aws::GameLift::Server::Model::UpdateReason::BACKFILL_FAILED:
         {
            // internal error - game session object unchanged
            _isQueuedForBackfill = false;
         }
         break;
      case Aws::GameLift::Server::Model::UpdateReason::BACKFILL_TIMED_OUT:
         {
            // matchmaker failed to find a backfill match within the time limit, session unchanged
            _isQueuedForBackfill = false;
         }
         break;
      case Aws::GameLift::Server::Model::UpdateReason::BACKFILL_CANCELLED:
         {
            // match backfill request was canceled by a call to StopMatchmaking (client) or StopMatchBackfill (server) - session unchanged.
            _isQueuedForBackfill = false;
         }
         break;
      case Aws::GameLift::Server::Model::UpdateReason::UNKNOWN:
         {
            // ???
            _isQueuedForBackfill = false;
         }
         break;
      }
   };

   FProcessParameters* params = new FProcessParameters();
   params->OnStartGameSession.BindLambda(onGameSession);
   params->OnUpdateGameSession.BindLambda(onUpdateGameSession);

   // OnProcessTerminate callback. GameLift invokes this before shutting down the instance 
   // that is hosting this game server to give it time to gracefully shut down on its own. 
   // In this example, we simply tell GameLift we are indeed going to shut down.
   params->OnTerminate.BindLambda(
      [=]()
      {
         UE_LOG(LogOSEDedicatedServer, Log, TEXT("Process Ending because we received OnTerminate from Gamelift"));
         FGameLiftGenericOutcome processEndingOutcome = _gameLiftSdkModule->ProcessEnding();
         GameliftSDKUtl::LogOutcome(TEXT("ProcessEnding"), processEndingOutcome);
      }
   );

   // HealthCheck callback. GameLift invokes this callback about every 60 seconds. By default, 
   // GameLift API automatically responds 'true'. A game can optionally perform checks on 
   // dependencies and such and report status based on this info. If no response is received  
   // within 60 seconds, health status is recorded as 'false'. 
   params->OnHealthCheck.BindLambda([&]()
   {
      // TODO: "You might report the server process as unhealthy if any external dependencies have failed or if metrics such as memory capacity fall outside a defined limit."
      // TODO: "Complete the health evaluation and respond to the callback within 60 seconds. If the Amazon GameLift service does not receive a response in that time, it will automatically consider the server process to be unhealthy."
      static int sHeatbeatCount = 0;
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("Heart beat %d..."), sHeatbeatCount);
      ++sHeatbeatCount;
      return true;
   });

   // Here, the game server tells GameLift what port it is listening on for incoming player 
   // connections. In this example, the port is hardcoded for simplicity. Since active game
   // that are on the same instance must have unique ports, you may want to assign port values
   // from a range, such as:
   params->port = GetPort();
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("Our port is %d"), params->port);

   //Here, the game server tells GameLift what set of files to upload when the game session 
   //ends. GameLift uploads everything specified here for the developers to fetch later.

   // if -log is passed into us use that log filepath, otherwise use the default TAT.log
   TArray<FString> logFiles;
   FString logFileName;
   if (FParse::Value(FCommandLine::Get(), TEXT("log="), logFileName))
   {
      FString customLogFile = FString::Printf(TEXT("../../Saved/Logs/%s"), *logFileName);
      logFiles.Add(customLogFile);
   }
   else
   {
      logFiles.Add(TEXT("../../Saved/Logs/TOW.log"));
   }
   check(logFiles.Num() != 0);
   params->logParameters = logFiles;

   UE_LOG(LogOSEDedicatedServer, Log, TEXT("Using log files:"));
   for (const FString& file : logFiles)
   {
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("   - %s"), *file);
   }

   // Call ProcessReady to tell GameLift this game server is ready to receive game sessions!
   FGameLiftGenericOutcome processReadyOutcome = _gameLiftSdkModule->ProcessReady(*params);
   GameliftSDKUtl::LogOutcome(TEXT("ProcessReady"), processReadyOutcome);
   check(processReadyOutcome.IsSuccess()); // I don't think this can fail...?
   
   _state = EOSEGameliftServerState::WaitingForGameSession;
}

void UOSEDedicatedServerManagerGameLift::_GameliftActivateGameSession()
{
   check(_gameLiftSdkModule);
   FGameLiftGenericOutcome outcome = _gameLiftSdkModule->ActivateGameSession();
   GameliftSDKUtl::LogOutcome(TEXT("ActivateGameSession"), outcome);
   if (outcome.IsSuccess())
   {
      // in the session
      _state = EOSEGameliftServerState::InGameSession;

      // time zero for waiting for players to get on the server
      _timeSinceGameSessionActivation = 0.0f;
   }
   else
   {
      // TODO: end the session if we fail to activate (what's this case??)
   }
}

void UOSEDedicatedServerManagerGameLift::_GameliftProcessEnding()
{
   check(_gameLiftSdkModule);

   // terminating!
   _state = EOSEGameliftServerState::Terminating;

   // Notifies the GameLift service that the server process is shutting down.This method should be called after all other
   // cleanup tasks, including shutting down all active game sessions. This method should exit with an exit code of 0; a non - zero
   // exit code results in an event message that the process did not exit cleanly.

   check(_gameLiftSdkModule);
   FGameLiftGenericOutcome outcome = _gameLiftSdkModule->ProcessEnding();
   GameliftSDKUtl::LogOutcome(TEXT("ProcessEnding"), outcome);

   int exitStatus = outcome.IsSuccess() ? 0 : 1; // doesn't look like FGameLiftGenericOutcome() has an error code, would be nice.
   FGenericPlatformMisc::RequestExitWithStatus(false, exitStatus);
}

bool UOSEDedicatedServerManagerGameLift::_GameliftAcceptPlayerSession(const FString& playerSessionId)
{
   check(_gameLiftSdkModule);
   FGameLiftGenericOutcome outcome = _gameLiftSdkModule->AcceptPlayerSession(playerSessionId);
   GameliftSDKUtl::LogOutcome(TEXT("AcceptPlayerSession"), outcome);
   return outcome.IsSuccess();
}

bool UOSEDedicatedServerManagerGameLift::_GameliftRemovePlayerSession(const FString& playerSessionId)
{  
   check(_gameLiftSdkModule);
   FGameLiftGenericOutcome outcome = _gameLiftSdkModule->RemovePlayerSession(playerSessionId);
   GameliftSDKUtl::LogOutcome(TEXT("RemovePlayerSession"), outcome);
   return outcome.IsSuccess();
}

void UOSEDedicatedServerManagerGameLift::_TickTimeSincePlayerSeen(float deltaTime)
{
   if (UWorld* world = GetWorld())
   {
      if (AGameStateBase* gs = world->GetGameState())
      {
         for(auto& entry : _playerInfo)
         {
            const FUniqueNetIdRepl& uniqueNetId = entry.Key;
            FOSEDedicatedServerGameliftPlayerInfo& playerInfo = entry.Value;
            
            APlayerState* foundPS = nullptr;
            for (APlayerState* ps : gs->PlayerArray)
            {
               if (!ps)
                  continue;

               if (ps->GetUniqueId() == uniqueNetId)
               {
                  foundPS = ps;
                  break;
               }
            }

            if (foundPS)
            {
               playerInfo.TimeSinceSeen = 0.0f;
            }
            else
            {
               playerInfo.TimeSinceSeen += deltaTime;
            }
         }
      }
   }

   TArray<FUniqueNetIdRepl> playersToRemove;
   for (auto& entry : _playerInfo)
   {
      const FUniqueNetIdRepl& uniqueNetId = entry.Key;
      FOSEDedicatedServerGameliftPlayerInfo& playerInfo = entry.Value;
      
      if(playerInfo.TimeSinceSeen >= GameliftSDKUtl::kRemoveFromSessionAfterPlayerNotSeenFor)
      {
         UE_LOG(LogOSEDedicatedServer, Log, TEXT("Removing player session %s because we have not seen it in %.02f seconds"), *uniqueNetId->ToString(), playerInfo.TimeSinceSeen);

         // remove the player session
         _GameliftRemovePlayerSession(playerInfo.PlayerSessionId);

         // nuke it from the map after we're done looping
         playersToRemove.Add(uniqueNetId);
      }
   }

   for(const FUniqueNetIdRepl& uniqueNetId : playersToRemove)
   {
      _playerInfo.Remove(uniqueNetId);
   }

   // TODO: hand off server ownership to the new owner if this owner is leaving
}
