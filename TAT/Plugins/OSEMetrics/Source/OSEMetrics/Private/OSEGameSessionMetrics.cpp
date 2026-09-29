// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEGameSessionMetrics.h"

// ose
#include "OSEMetrics.h"
#include "OSEMetricsSystem.h"
#include "OSEGameSessionMetricsSubsystem.h"

// ue
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Misc/DefaultValueHelper.h"


static const FName kOSEApplicationMetricsGroupName = FName("Application");
static const FName kOSEGameSessionMetricsGroupName = FName("Game");


namespace MetricsHelpers
{
   inline AGameStateBase* FindGameState(UObject* worldContext)
   {
      UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull);
      return world ? world->GetGameState() : nullptr;
   }

   inline AGameModeBase* FindGameMode(UObject* worldContext)
   {
      UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull);
      return world ? world->GetAuthGameMode() : nullptr;
   }

   inline AGameSession* FindGameSession(UObject* worldContext)
   {
      AGameModeBase* gameMode = FindGameMode(worldContext);
      return gameMode ? gameMode->GameSession : nullptr;
   }
}


FOSEGameSessionMetrics::FOSEGameSessionMetrics(UGameInstance* gameInstance, const TSharedPtr<FOSEMetricsSystem>& metricsSystem)
   : _gameInstance(gameInstance)
   , _metricsSystem(metricsSystem)
{
   check(_metricsSystem != nullptr);
   _metricsSystem->AddMetricsGroup(kOSEApplicationMetricsGroupName);
   _metricsSystem->AddMetricsGroup(kOSEGameSessionMetricsGroupName);
}

FOSEGameSessionMetrics::~FOSEGameSessionMetrics()
{
}

void FOSEGameSessionMetrics::InstallApplicationMetrics()
{
   check(_metricsSystem.IsValid());

   _metricsSystem->AddMetric(kOSEApplicationMetricsGroupName, FName("Process"),
      [](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         result->Values.Add(TEXT("TimeSecondsSinceProcessStart"), MakeShared<FJsonValueNumber>(FPlatformTime::Seconds() - GStartTime));
         return result;
      });

   _metricsSystem->AddMetric(kOSEApplicationMetricsGroupName, FName("System"),
      [](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         
         result->Values.Add(TEXT("PlatformName"), MakeShared<FJsonValueString>(FPlatformProperties::PlatformName()));

         result->Values.Add(TEXT("CPUCoreCount"), MakeShared<FJsonValueNumber>(FPlatformMisc::NumberOfCores()));

#if !PLATFORM_WINDOWS
         // GetStats can be slow on Windows, and this is primarily here for Linux servers anyway
         const FPlatformMemoryStats memoryStats = FPlatformMemory::GetStats();
         result->Values.Add(TEXT("MemoryAvailablePhysicalBytes"), MakeShared<FJsonValueNumber>(memoryStats.AvailablePhysical));
         result->Values.Add(TEXT("MemoryUsedPhysicalBytes"), MakeShared<FJsonValueNumber>(memoryStats.UsedPhysical));
         result->Values.Add(TEXT("MemoryPeakUsedPhysicalBytes"), MakeShared<FJsonValueNumber>(memoryStats.PeakUsedPhysical));
#endif

         return result;
      });
}

void FOSEGameSessionMetrics::InstallGameSessionMetrics()
{
   check(_metricsSystem.IsValid());

   UGameInstance* gameInstance = _gameInstance.Get();
   if (gameInstance == nullptr)
   {
      UE_LOG(LogOSEMetrics, Error, TEXT("Failed to install game session metrics - the game instance is null"));
      return;
   }

   TWeakPtr<FOSEGameSessionMetrics> weakThis = AsWeak();

   _metricsSystem->AddMetric(kOSEGameSessionMetricsGroupName, FName("FrameTimes"),
      [weakThis](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         UWorld* world = GEngine->GetWorldFromContextObject(GetGameInstance(weakThis), EGetWorldErrorMode::ReturnNull);
         if (world == nullptr)
         {
            return nullptr;
         }

         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();

         result->Values.Add(TEXT("CurrentGameDeltaSeconds"), MakeShared<FJsonValueNumber>(world->DeltaTimeSeconds));
         result->Values.Add(TEXT("CurrentRealDeltaSeconds"), MakeShared<FJsonValueNumber>(world->DeltaRealTimeSeconds));

         if (UOSEGameSessionMetricsSubsystem* metricsSubsystem = world->GetSubsystem<UOSEGameSessionMetricsSubsystem>())
         {
            static constexpr double defaultTimespanSeconds = 1.0;
            const double currentTime = FPlatformTime::Seconds();
            const double timespanSeconds = OSEMetricsUtils::TryParseQueryParamValue<double>(queryParams, TEXT("Timespan")).Get(defaultTimespanSeconds);
            result->Values.Add(TEXT("GameFrameStats"), MakeShared<FJsonValueObject>(metricsSubsystem->GetGameDeltaSecondsStats(currentTime, timespanSeconds).ToJsonObject()));
            result->Values.Add(TEXT("RealFrameStats"), MakeShared<FJsonValueObject>(metricsSubsystem->GetRealDeltaSecondsStats(currentTime, timespanSeconds).ToJsonObject()));
         }

         return result;
      });

   FCoreUObjectDelegates::PreLoadMapWithContext.AddSPLambda(this, [this](const FWorldContext& worldContext, const FString& mapName)
   {
      FMapLoadMetrics& mapMetrics = _mapLoadMetrics.AddDefaulted_GetRef();
      mapMetrics.IsMapLoading = true;
      mapMetrics.MapName = mapName;
      mapMetrics.LoadStartTime = FPlatformTime::Seconds();
      mapMetrics.LoadDuration = 0.0;
   });

   FCoreUObjectDelegates::PostLoadMapWithWorld.AddSPLambda(this, [this](UWorld* loadedWorld)
   {
      if (_mapLoadMetrics.IsEmpty())
      {
         return;
      }
      FMapLoadMetrics& mapMetrics = _mapLoadMetrics.Last();
      if (!mapMetrics.IsMapLoading || mapMetrics.LoadStartTime == 0)
      {
         return;
      }
      mapMetrics.IsMapLoading = false;
      mapMetrics.LoadDuration = FPlatformTime::Seconds() - mapMetrics.LoadStartTime;
   });

   _metricsSystem->AddMetric(kOSEGameSessionMetricsGroupName, FName("World"),
      [weakThis](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         TSharedPtr<FOSEGameSessionMetrics> self = weakThis.Pin();
         if (!self.IsValid())
         {
            return nullptr;
         }

         UWorld* world = GEngine->GetWorldFromContextObject(GetGameInstance(weakThis), EGetWorldErrorMode::ReturnNull);
         if (world == nullptr)
         {
            return nullptr;
         }

         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         result->Values.Add(TEXT("MapName"), MakeShared<FJsonValueString>(world->GetMapName()));

         const bool mapLoadInProgress = !self->_mapLoadMetrics.IsEmpty() ? self->_mapLoadMetrics.Last().IsMapLoading : false;
         result->Values.Add(TEXT("MapLoadInProgress"), MakeShared<FJsonValueBoolean>(mapLoadInProgress));

         // Add in all map load events (and how long each took)
         if (!self->_mapLoadMetrics.IsEmpty())
         {
            auto mapLoadMetricsToJsonObject = [](const FMapLoadMetrics& mapMetrics) -> TSharedPtr<FJsonObject>
            {
               TSharedPtr<FJsonObject> obj = MakeShared<FJsonObject>();
               obj->Values.Add(TEXT("MapName"), MakeShared<FJsonValueString>(mapMetrics.MapName));
               obj->Values.Add(TEXT("IsLoading"), MakeShared<FJsonValueBoolean>(mapMetrics.IsMapLoading));
               obj->Values.Add(TEXT("LoadDuration"), MakeShared<FJsonValueNumber>(mapMetrics.LoadDuration));
               // Map load start time (relative to the time the process was launched)
               obj->Values.Add(TEXT("LoadStartTime"), MakeShared<FJsonValueNumber>(mapMetrics.LoadStartTime - GStartTime));
               // Map load end time (relative to the time the process was launched)
               const double loadEndTime = !mapMetrics.IsMapLoading ? ((mapMetrics.LoadStartTime + mapMetrics.LoadDuration) - GStartTime) : 0.0;
               obj->Values.Add(TEXT("LoadEndTime"), MakeShared<FJsonValueNumber>(loadEndTime));
               return obj;
            };

            TArray<TSharedPtr<FJsonValue>> mapLoads;
            for (const FMapLoadMetrics& mapMetrics : self->_mapLoadMetrics)
            {
               mapLoads.Add(MakeShared<FJsonValueObject>(mapLoadMetricsToJsonObject(mapMetrics)));
            }
            result->Values.Add(TEXT("MapLoads"), MakeShared<FJsonValueArray>(MoveTemp(mapLoads)));
         }

         TArray<TSharedPtr<FJsonValue>> connections;
         if (world->NetDriver != nullptr)
         {
            for (UNetConnection* conn : world->NetDriver->ClientConnections)
            {
               if (conn == nullptr)
               {
                  connections.Add(MakeShared<FJsonValueNull>());
                  continue;
               }
               TSharedPtr<FJsonObject> connInfo = MakeShared<FJsonObject>();
               connInfo->Values.Add(TEXT("PlayerId"), conn->PlayerId.ToJson());

               if (conn->RemoteAddr)
               {
                  connInfo->Values.Add(TEXT("RemoteAddr"), MakeShared<FJsonValueString>(conn->RemoteAddr->ToString(true)));
               }
               else
               {
                  connInfo->Values.Add(TEXT("RemoteAddr"), MakeShared<FJsonValueNull>());
               }

               connInfo->Values.Add(TEXT("ConnectTime"), MakeShared<FJsonValueNumber>(conn->GetConnectTime()));

               connInfo->Values.Add(TEXT("Channels.Num"), MakeShared<FJsonValueNumber>(conn->Channels.Num()));

               connInfo->Values.Add(TEXT("StatPeriod"), MakeShared<FJsonValueNumber>(conn->StatPeriod));
               connInfo->Values.Add(TEXT("StatUpdateTime"), MakeShared<FJsonValueNumber>(conn->StatUpdateTime));

               auto packetLossToJson = [](const UNetConnection::FNetConnectionPacketLoss& val) -> TSharedPtr<FJsonValue>
               {
                  TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
                  result->Values.Add(TEXT("Pct"), MakeShared<FJsonValueNumber>(val.GetLossPercentage()));
                  result->Values.Add(TEXT("AvgPct"), MakeShared<FJsonValueNumber>(val.GetAvgLossPercentage()));
                  return MakeShared<FJsonValueObject>(result);
               };
               connInfo->Values.Add(TEXT("InPacketsLossPercentage"), packetLossToJson(conn->GetInLossPercentage()));
               connInfo->Values.Add(TEXT("OutPacketsLossPercentage"), packetLossToJson(conn->GetOutLossPercentage()));

               connInfo->Values.Add(TEXT("AvgLag"), MakeShared<FJsonValueNumber>(conn->AvgLag));

               // /** bytes sent/received on this connection (accumulated during a StatPeriod) */
               connInfo->Values.Add(TEXT("InBytes"), MakeShared<FJsonValueNumber>(conn->InBytes));
               connInfo->Values.Add(TEXT("OutBytes"), MakeShared<FJsonValueNumber>(conn->OutBytes));
               // /** total bytes sent/received on this connection */
               connInfo->Values.Add(TEXT("InTotalBytes"), MakeShared<FJsonValueNumber>(conn->InTotalBytes));
               connInfo->Values.Add(TEXT("OutTotalBytes"), MakeShared<FJsonValueNumber>(conn->OutTotalBytes));
               // /** packets sent/received on this connection (accumulated during a StatPeriod) */
               connInfo->Values.Add(TEXT("InPackets"), MakeShared<FJsonValueNumber>(conn->InPackets));
               connInfo->Values.Add(TEXT("OutPackets"), MakeShared<FJsonValueNumber>(conn->OutPackets));
               // /** Packets received in the current tick */
               // connInfo->Values.Add(TEXT("InPacketsThisFrame"), MakeShared<FJsonValueNumber>(conn->InPacketsThisFrame));
               // /** Packets sent in the current tick */
               // connInfo->Values.Add(TEXT("OutPacketsThisFrame"), MakeShared<FJsonValueNumber>(conn->OutPacketsThisFrame));
               // /** total packets sent/received on this connection */
               connInfo->Values.Add(TEXT("InTotalPackets"), MakeShared<FJsonValueNumber>(conn->InTotalPackets));
               connInfo->Values.Add(TEXT("OutTotalPackets"), MakeShared<FJsonValueNumber>(conn->OutTotalPackets));
               // /** bytes sent/received on this connection (per second) - these are from previous StatPeriod interval */
               connInfo->Values.Add(TEXT("InBytesPerSecond"), MakeShared<FJsonValueNumber>(conn->InBytesPerSecond));
               connInfo->Values.Add(TEXT("OutBytesPerSecond"), MakeShared<FJsonValueNumber>(conn->OutBytesPerSecond));
               // /** packets sent/received on this connection (per second) - these are from previous StatPeriod interval */
               connInfo->Values.Add(TEXT("InPacketsPerSecond"), MakeShared<FJsonValueNumber>(conn->InPacketsPerSecond));
               connInfo->Values.Add(TEXT("OutPacketsPerSecond"), MakeShared<FJsonValueNumber>(conn->OutPacketsPerSecond));
               // /** packets lost on this connection (accumulated during a StatPeriod) */
               connInfo->Values.Add(TEXT("InPacketsLost"), MakeShared<FJsonValueNumber>(conn->InPacketsLost));
               connInfo->Values.Add(TEXT("OutPacketsLost"), MakeShared<FJsonValueNumber>(conn->OutPacketsLost));
               // /** total packets lost on this connection */
               connInfo->Values.Add(TEXT("InTotalPacketsLost"), MakeShared<FJsonValueNumber>(conn->InTotalPacketsLost));
               connInfo->Values.Add(TEXT("OutTotalPacketsLost"), MakeShared<FJsonValueNumber>(conn->OutTotalPacketsLost));
               // /** total acks sent on this connection */
               connInfo->Values.Add(TEXT("OutTotalAcks"), MakeShared<FJsonValueNumber>(conn->OutTotalAcks));
               // /** Delayed RPCs and the total average frame delay */
               connInfo->Values.Add(TEXT("TotalDelayedRPCs"), MakeShared<FJsonValueNumber>(conn->TotalDelayedRPCs));
               connInfo->Values.Add(TEXT("TotalDelayedRPCsFrameCount"), MakeShared<FJsonValueNumber>(conn->TotalDelayedRPCsFrameCount));

               connections.Add(MakeShared<FJsonValueObject>(connInfo));
            }
         }
         result->Values.Add(TEXT("NetDriver.ClientConnections"), MakeShared<FJsonValueArray>(connections));
         return result;
      });

   _metricsSystem->AddMetric(kOSEGameSessionMetricsGroupName, FName("GameMode"),
      [weakThis](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         TSharedPtr<FOSEGameSessionMetrics> self = weakThis.Pin();
         if (!self.IsValid())
         {
            return nullptr;
         }
         AGameModeBase* gameMode = MetricsHelpers::FindGameMode(GetGameInstance(weakThis));
         if (gameMode == nullptr)
         {
            return nullptr;
         }
         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         result->Values.Add(TEXT("Name"), MakeShared<FJsonValueString>(gameMode->GetName()));
         result->Values.Add(TEXT("NumPlayers"), MakeShared<FJsonValueNumber>(gameMode->GetNumPlayers()));
         result->Values.Add(TEXT("NumSpectators"), MakeShared<FJsonValueNumber>(gameMode->GetNumSpectators()));
         result->Values.Add(TEXT("IsPaused"), MakeShared<FJsonValueBoolean>(gameMode->IsPaused()));
         if (self->_extendGameModeMetrics)
         {
            self->_extendGameModeMetrics(gameMode, result);
         }
         return result;
      });

   _metricsSystem->AddMetric(kOSEGameSessionMetricsGroupName, FName("GameSession"),
      [weakThis](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         AGameSession* gameSession = MetricsHelpers::FindGameSession(GetGameInstance(weakThis));
         if (gameSession == nullptr)
         {
            return nullptr;
         }
         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         FJoinabilitySettings settings{};
         gameSession->GetSessionJoinability(gameSession->SessionName, settings);
         result->Values.Add(TEXT("SessionName"), MakeShared<FJsonValueString>(settings.SessionName.ToString()));
         result->Values.Add(TEXT("PublicSearchable"), MakeShared<FJsonValueBoolean>(settings.bPublicSearchable));
         result->Values.Add(TEXT("AllowInvites"), MakeShared<FJsonValueBoolean>(settings.bAllowInvites));
         result->Values.Add(TEXT("JoinViaPresence"), MakeShared<FJsonValueBoolean>(settings.bJoinViaPresence));
         result->Values.Add(TEXT("JoinViaPresenceFriendsOnly"), MakeShared<FJsonValueBoolean>(settings.bJoinViaPresenceFriendsOnly));
         result->Values.Add(TEXT("MaxPlayers"), MakeShared<FJsonValueNumber>(settings.MaxPlayers));
         result->Values.Add(TEXT("MaxPartySize"), MakeShared<FJsonValueNumber>(settings.MaxPartySize));
         result->Values.Add(TEXT("MaxSpectators"), MakeShared<FJsonValueNumber>(gameSession->MaxSpectators));
         return result;
      });

   _metricsSystem->AddMetric(kOSEGameSessionMetricsGroupName, FName("GameState"),
      [weakThis](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         TSharedPtr<FOSEGameSessionMetrics> self = weakThis.Pin();
         if (!self.IsValid())
         {
            return nullptr;
         }
         AGameStateBase* gameState = MetricsHelpers::FindGameState(GetGameInstance(weakThis));
         if (gameState == nullptr)
         {
            return nullptr;
         }
         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         result->Values.Add(TEXT("ServerWorldTimeSeconds"), MakeShared<FJsonValueNumber>(gameState->GetServerWorldTimeSeconds()));
         TArray<TSharedPtr<FJsonValue>> playerStates;
         for (APlayerState* playerState : gameState->PlayerArray)
         {
            if (playerState == nullptr)
            {
               playerStates.Add(MakeShared<FJsonValueNull>());
               continue;
            }
            TSharedPtr<FJsonObject> player = MakeShared<FJsonObject>();
            player->Values.Add(TEXT("PlayerName"), MakeShared<FJsonValueString>(playerState->GetPlayerName()));
            player->Values.Add(TEXT("UniqueId"), playerState->GetUniqueId().ToJson());
            player->Values.Add(TEXT("PlayerId"), MakeShared<FJsonValueNumber>(playerState->GetPlayerId()));
            player->Values.Add(TEXT("ExactPing"), MakeShared<FJsonValueNumber>(playerState->ExactPing));
            player->Values.Add(TEXT("IsSpectator"), MakeShared<FJsonValueBoolean>(playerState->IsSpectator()));
            player->Values.Add(TEXT("IsABot"), MakeShared<FJsonValueBoolean>(playerState->IsABot()));
            player->Values.Add(TEXT("IsInactive"), MakeShared<FJsonValueBoolean>(playerState->IsInactive()));
            player->Values.Add(TEXT("StartTime"), MakeShared<FJsonValueNumber>(playerState->GetStartTime()));

            if (self->_extendPlayerStateMetrics)
            {
               self->_extendPlayerStateMetrics(playerState, player);
            }

            playerStates.Add(MakeShared<FJsonValueObject>(player));
         }
         result->Values.Add(TEXT("Players"), MakeShared<FJsonValueArray>(playerStates));

         if (self->_extendGameStateMetrics)
         {
            self->_extendGameStateMetrics(gameState, result);
         }

         return result;
      });
}


// static
UGameInstance* FOSEGameSessionMetrics::GetGameInstance(const TWeakPtr<FOSEGameSessionMetrics>& weakThis)
{
   TSharedPtr<FOSEGameSessionMetrics> self = weakThis.Pin();
   return self.IsValid() ? self->_gameInstance.Get() : nullptr;
}
