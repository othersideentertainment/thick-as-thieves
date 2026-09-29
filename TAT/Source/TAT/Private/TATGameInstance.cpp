// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATGameInstance.h"

// tat
#include "Analytics/TATAnalyticsMgr.h" //+jmb: deprecated
#include "Character/TATTeams.h"
#include "Developer/TATEditorSettings.h"
#include "GameFramework/TATOnlineSessionClient.h"
#include "GameFramework/TATTravelMgr.h"
#include "Online/TATGameSession.h"
#include "Player/TATPlayerController.h"
#include "UI/TATNavigationConfig.h"
#include "Settings/TATMatchSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"
#include "Loot/TATLootInventory.h"

// ose
#include "OSEMetricsSystem.h"
#include "OSEGameSessionMetrics.h"
#include "OSEMetricsSettings.h"
#include "OSEMetricsUtils.h"
#include "OSEMetricsOutputFile.h"
#include "OSEMetricsOutputHTTPServer.h"

// ue5
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "Misc/NetworkVersion.h"
#include "OnlineSubsystemUtils/Public/OnlineSubsystemUtils.h"
#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#endif

#include "Common/TATServerLogFlushHelpers.h"
#include "UI/TATUIFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameInstance)

#define SAVE_DATA_BEFORE_JOIN 1

DEFINE_LOG_CATEGORY_STATIC(LogTATGameInstance, Log, All);

namespace GameInstanceHelpers
{
   static void CrashTheGame()
   {
      AActor* actor = nullptr; // intentionally null to crash the game!
      actor->GetName();
   }
   
   static void PossiblyCrashAfterDelay(const UGameInstance& gameInstance)
   {
      float delay = 0;
      if (FParse::Value(FCommandLine::Get(), TEXT("-CrashAfterDelay="), delay) && delay > 0)
      {
         FTimerHandle timerHandle;
         gameInstance.GetTimerManager().SetTimer(timerHandle, TFunction<void()>(CrashTheGame), delay, false);
      }
   }

#if WITH_EDITOR
   static void TryApplyLobbyOverrides(UTATGameInstance& instance)
   {
      const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
      if (!editorSettings.OverrideLobbySettings)
      {
         return;
      }

      FTATMapNodeSettings result = instance.GetMapNodeSettings();
      bool dirty = false;

      if (const FTATMapSettings* mapSettings = UTATProjectSettings::Get().FindMapSettings(editorSettings.LobbyMapTag))
      {
         result.Map = mapSettings->Map;
         result.MapName = mapSettings->MapName;
         result.MapDisplayName = mapSettings->MapDisplayName;
         dirty = true;
      }

      if (editorSettings.LobbyDifficulty.IsSet())
      {
         result.Difficulty = *editorSettings.LobbyDifficulty;
         dirty = true;
      }

      if (dirty)
      {
         instance.SetMapNodeSettings(result);
      }
   }
#endif
}

/* static */
UTATGameInstance& UTATGameInstance::Get(const UObject* contextObject)
{
   check(IsValid(contextObject));
   UTATGameInstance* gameInstance = CastChecked<UTATGameInstance>(contextObject->GetWorld()->GetGameInstance());
   return *gameInstance;
}

/* static */
UTATGameInstance* UTATGameInstance::GetTATGameInstance(const UObject* contextObject)
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
   {
      return world->GetGameInstance<UTATGameInstance>();
   }
   return nullptr;
}

void UTATGameInstance::Init()
{
   Super::Init();

   _InitNetworkVersionOverride();

   TATServerLogFlushHelpers::TryInitialize(GetTimerManager());
   GameInstanceHelpers::PossiblyCrashAfterDelay(*this);

#if WITH_EDITOR
   // Automated tests run on the editor, which in turn uses the net mode from the ULevelEditorPlaySettings
   // To avoid inconsistent test setups, we pass in a flag to force the net mode to something known
   // Then we can run the right set of tests for the right fake net mode
   if (FParse::Param(FCommandLine::Get(), TEXT("ForceStandalone")))
   {
      ULevelEditorPlaySettings* EditorUserSettings = GetMutableDefault<ULevelEditorPlaySettings>();
      EditorUserSettings->SetPlayNetMode(EPlayNetMode::PIE_Standalone);
   }
   if (FParse::Param(FCommandLine::Get(), TEXT("ForceListen")))
   {
      ULevelEditorPlaySettings* EditorUserSettings = GetMutableDefault<ULevelEditorPlaySettings>();
      EditorUserSettings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
   }
   if (FParse::Param(FCommandLine::Get(), TEXT("ForceClient")))
   {
      ULevelEditorPlaySettings* EditorUserSettings = GetMutableDefault<ULevelEditorPlaySettings>();
      EditorUserSettings->SetPlayNetMode(EPlayNetMode::PIE_Client);
   }
   
   if (GEngine->IsEditor() && !GetWorld()->IsNetMode(NM_Client))
   {
      GameInstanceHelpers::TryApplyLobbyOverrides(*this);
   }
#endif
#if 1 //!UE_BUILD_SHIPPING
   {
      // want to allow this in perf tests
      int32 seedOverride;
      if (FParse::Value(FCommandLine::Get(), TEXT("-ForceSeed="), seedOverride))
      {
         GetMutableDefault<UTATEditorSettings>()->WorldRandomizationSeed = seedOverride;
      }
   }
#endif

   // Start the metrics server if configured to do so
   UOSEMetricsSettings::FMetricsConfig metricsConfig{};
   if (UOSEMetricsSettings::GetMetricsServerConfig(metricsConfig))
   {
      const UOSEMetricsSettings& metricsSettings = UOSEMetricsSettings::Get();

      _metricsSystem = MakeShared<FOSEMetricsSystem>();

      if (metricsConfig.HTTPServerOutput)
      {
         constexpr uint16 fallbackDefaultPort = 9071;
         _metricsSystem->AddOutput(MakeShared<FOSEMetricsOutputHTTPServer>(metricsConfig.Port.Get(fallbackDefaultPort)));
      }

      if (metricsConfig.FileOutput)
      {
         _metricsSystem->AddOutput(MakeShared<FOSEMetricsOutputFile>(
            metricsSettings.FileOutputQueryIntervalSeconds,
            metricsSettings.FileOutputFlushIntervalSeconds,
            metricsSettings.FileOutputSaveToProfilingDir));
      }

      _gameSessionMetrics = MakeShared<FOSEGameSessionMetrics>(this, _metricsSystem);
      _gameSessionMetrics->InstallApplicationMetrics();
      _gameSessionMetrics->InstallGameSessionMetrics();

      _InstallMetricsHandlers();

      _metricsSystem->Enable();
   }

   // Game Instance Globals:
   // (systems that are required to live cross-map only, please)
   check(!_travelMgr);
   _travelMgr = NewObject<UTATTravelMgr>();
   _travelMgr->Init(this);

   check(!_analyticsMgr);
   _analyticsMgr = NewObject<UTATAnalyticsMgr>(this);
   _analyticsMgr->Init();

   check(!_matchSettings);
   _matchSettings = NewObject<UTATMatchSettingsBase>(this, UTATProjectSettings::Get().GetMatchSettingsClass());

   // TAT project team attitude solver static init
   UTATTeamAttitudeSolver::Init();

   FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTATGameInstance::_OnPostLoadMap);
   OnNotifyPreClientTravel().AddUObject(this, &UTATGameInstance::_HandlePreClientTravel);

   // compile flag to turn screen messages off, which we can flip on for MS builds
   // that we still want to release as development builds for console/cheats but to look a bit nicer.
#if TAT_DISABLE_SCREEN_MESSAGES
   GAreScreenMessagesEnabled = false;
#endif

   // Setup our custom navigation config
   if (FSlateApplication::IsInitialized())
   {
      FSlateApplication::Get().SetNavigationConfig(FTATNavigationConfig::GetInstance());
      FSlateApplication::Get().SetHandleDeviceInputWhenApplicationNotActive(true);
   }

   const UTATProjectSettings& settings = *GetDefault<UTATProjectSettings>();

   if (UStringTable* stringTable = settings.SettingsStringTable.LoadSynchronous())
   {
      const FName tableID = stringTable->GetStringTableId();
      if (!FStringTableRegistry::Get().FindStringTable(tableID).IsValid())
      {
         FStringTableRegistry::Get().RegisterStringTable(tableID, stringTable->GetMutableStringTable());
      }
   }
   
   if (IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld()))
   {
      if (sessionInterface.IsValid())
      {
         // Bind to the invite accepted delegate
         sessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
            FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::_OnSessionUserInviteAccepted)
         );
      }
   }
}

void UTATGameInstance::OnWorldChanged(UWorld* oldWorld, UWorld* newWorld)
{
   Super::OnWorldChanged(oldWorld, newWorld);

   if (_travelMgr)
   {
      _travelMgr->OnWorldChanged(oldWorld, newWorld);
   }
}

TSubclassOf<UOnlineSession> UTATGameInstance::GetOnlineSessionClass()
{
   return UTATOnlineSessionClient::StaticClass();
}

void UTATGameInstance::Shutdown()
{
   _travelMgr->Shutdown();
   _analyticsMgr->Shutdown();
   
   if (IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld()))
   {
      if (sessionInterface.IsValid())
      {
         sessionInterface->OnSessionUserInviteAcceptedDelegates.RemoveAll(this);
      }
   }

   // this will shutdown any game instance subsystems, including NetJobMgr, so let's do it last.
   // this allows any code above us to insert net jobs to flush out upon app close.
   Super::Shutdown();
}

void UTATGameInstance::ShowLoadingScreen()
{
   GetTravelMgr().ShowLoadingScreen();
}

void UTATGameInstance::HideLoadingScreen()
{
   GetTravelMgr().HideLoadingScreen();
}

void UTATGameInstance::UpdateMatchSettings(const TArray<uint8>& newMatchSettingsData, bool autoUpdateGameState)
{
   ensure(newMatchSettingsData.Num() > 0);
   check(_matchSettings != nullptr);

   if (!ensure(_matchSettings->DeserializeFromByteArray(newMatchSettingsData)))
   {
      return;
   }

   if (autoUpdateGameState)
   {
      if (ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
      {
         // Technically we could pass newMatchSettingsData directly to the game state so it will sync to all clients, but I'm wary of
         // fully trusting that bytes sent from the client is totally safe to send directly to all other clients.
         // Just to be safe, we'll reserialize the (now validated and clamped) match settings data and pass that data to the game state to be replicated to all clients.
         TArray<uint8> matchSettingsData;
         _matchSettings->SerializeToByteArray(matchSettingsData);
         gameState->AuthorityNotifyMatchSettingsDataUpdated(MoveTemp(matchSettingsData));
      }
   }

   OnMatchSettingsUpdated.Broadcast(_matchSettings);
}

void UTATGameInstance::AuthorityNotifyCheatUpdatedMatchSettings()
{
#if OSE_CHEATS_ENABLED
   ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>();
   check(gameState != nullptr);
   check(gameState->HasAuthority());
   TArray<uint8> matchSettingsData;
   _matchSettings->SerializeToByteArray(matchSettingsData);
   gameState->AuthorityNotifyMatchSettingsDataUpdated(MoveTemp(matchSettingsData));
   OnMatchSettingsUpdated.Broadcast(_matchSettings);
#endif
}

void UTATGameInstance::SetMapNodeSettings(const FTATMapNodeSettings& newMapNodeSettings)
{
   _mapNodeSettings = newMapNodeSettings;
   OnMapNodeSettingsUpdated.Broadcast(_mapNodeSettings);
}

int32 UTATGameInstance::GetMaxNumPlayersForSession() const
{
   // NB: For now, this value is set by an ini config, and we want to expose it to Blueprints so we can use it when creating sessions
   // In the future, this may depend on which map we want to load, but that will require tweaks to how the OSS flow is done
   int32 maxNumPlayers = ATATGameSession::StaticClass()->GetDefaultObject<ATATGameSession>()->MaxPlayers;

   return maxNumPlayers;
}

void UTATGameInstance::SetNetworkFailuresConsumed()
{
   _showNetworkFailure = false;
   _showTravelFailure = false;
}

void UTATGameInstance::ClearParty()
{
   _partyMembers.Reset();
}

void UTATGameInstance::AddPlayerToParty(const FUniqueNetIdRepl& playerId)
{
   if (playerId.IsValid())
   {
      _partyMembers.AddUnique(playerId);
   }
}

int32 UTATGameInstance::GetPartySize() const
{
   return _partyMembers.Num();
}

bool UTATGameInstance::IsPlayerInParty(const FUniqueNetIdRepl& playerId) const
{
   return _partyMembers.Contains(playerId);
}

int32 UTATGameInstance::TryPredictPlayerIdForPartyMember(const FUniqueNetIdRepl& uniqueId) const
{
   const int32 indexInParty = _partyMembers.Find(uniqueId);
   if (indexInParty != INDEX_NONE)
   {
      return indexInParty + 1;
   }

   return INDEX_NONE;
}

void UTATGameInstance::ClearContractSelections()
{
   _contractSelections.Reset();
}

void UTATGameInstance::SetContractSelections(FTATPartyContracts questSelections)
{
   _contractSelections = MoveTemp(questSelections);
}

const FTATPartyContracts& UTATGameInstance::GetContractSelections() const
{
   return _contractSelections;
}

void UTATGameInstance::ReturnToThievesDen_Implementation(const FText& returnReason)
{
}

#if SAVE_DATA_BEFORE_JOIN
bool UTATGameInstance::JoinPendingSession()
{
   _travelReadyToTravel = true;
   return _JoinPendingSession();
}

bool UTATGameInstance::JoinSession(ULocalPlayer* localPlayer, const FOnlineSessionSearchResult& searchResult)
{
   UE_LOG(LogTATGameInstance, Verbose, TEXT("Joining session by search results"));
   if (!localPlayer)
      return false;
   _travelNextPlayerIdx = localPlayer->GetLocalPlayerIndex();
   _travelSearchResult = searchResult;
   if (_travelReadyToTravel)
   {
      return _JoinPendingSession();
   }
   return false;
}
#else
void UTATGameInstance::LoginFlowComplete()
{
   _travelReadyToTravel = true;
   _TravelToSession();
}

bool UTATGameInstance::JoinSession(ULocalPlayer* localPlayer, const FOnlineSessionSearchResult& searchResult)
{
   UE_LOG(LogTATGameInstance, Verbose, TEXT("Joining session by search results"));
   if (!localPlayer)
      return false;
   _travelNextPlayerIdx = localPlayer->GetLocalPlayerIndex();
   _travelSearchResult = searchResult;
   if (IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld()))
   {
      sessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &ThisClass::_OnJoinSessionComplete);
      return sessionInterface->JoinSession(_travelNextPlayerIdx, NAME_GameSession, _travelSearchResult);
   }  
   return false;
}

#endif

APlayerController* UTATGameInstance::_GetPlayerControllerForControllerId(const int32 controllerId) const
{
   auto player = FindLocalPlayerFromControllerId(controllerId);
   if (player)
      return player->GetPlayerController(GetWorld());
   return GetFirstLocalPlayerController();
}

APlayerController* UTATGameInstance::_GetPlayerControllerForLocalIndex(const int32 localPlayerIndex) const
{
   auto player = GetLocalPlayerByIndex(localPlayerIndex);
   if (player)
      return player->GetPlayerController(GetWorld());
   return GetFirstLocalPlayerController();
}

void UTATGameInstance::_OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 controllerId, FUniqueNetIdPtr /*UserId*/, const FOnlineSessionSearchResult& searchResult)
{
   if (bWasSuccessful)
   {
      if (!searchResult.IsValid())
      {
         const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
         if (projectSettings.JoinSessionFailedNotFoundToastTag.IsValid())
         {
            UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForControllerId(controllerId), projectSettings.JoinSessionFailedNotFoundToastTag, projectSettings.JoinSessionFailedNotFoundToastMessage);
         }         
      }
   }
   else
   {
      const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
      if (projectSettings.JoinSessionFailedToastTag.IsValid())
      {
         UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForControllerId(controllerId), projectSettings.JoinSessionFailedToastTag, projectSettings.JoinSessionFailedToastMessage);
      }
   }
}

void UTATGameInstance::_OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result)
{ 
   if (result != EOnJoinSessionCompleteResult::Success)
   {
      UE_LOG(LogTATGameInstance, Error, TEXT("ClientJoinOnlineSession: Failed to join server session (%s)"), LexToString(result));
      const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
      if (projectSettings.JoinSessionFailedToastTag.IsValid())
      {
         UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForLocalIndex(_travelNextPlayerIdx), projectSettings.JoinSessionFailedToastTag, projectSettings.JoinSessionFailedToastMessage);
      }
      _TravelResetParameters();
      return;
   }
   _travelNextSessionName = sessionName;
   _TravelToSession();
   if (IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld()))
   {
      sessionInterface->OnJoinSessionCompleteDelegates.RemoveAll(this);
   }
}

bool UTATGameInstance::_JoinPendingSession()
{
   if (!_travelReadyToTravel)
      return false;
   if (_travelNextPlayerIdx == INDEX_NONE)
      return false;
   if (!_travelSearchResult.IsValid())
      return false;
   
   if(const UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
      ensure(saveGame) && saveGame->GetFtueState() != ETATSavedFtueState::Complete)
   {
      const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
      if (projectSettings.IncompleteTutorialInviteToastTag.IsValid())
      {
         UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForLocalIndex(_travelNextPlayerIdx), projectSettings.IncompleteTutorialInviteToastTag, projectSettings.IncompleteTutorialInviteToastMessage);
      }
      return false;
   }
   
   UE_LOG(LogTATGameInstance, Verbose, TEXT("Joining pending session after login and data load complete"));

   if (IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld()))
   {
      sessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &ThisClass::_OnJoinSessionComplete);
      bool result = sessionInterface->JoinSession(_travelNextPlayerIdx, NAME_GameSession, _travelSearchResult);
      _travelSearchResult = FOnlineSessionSearchResult();
      check(!_travelSearchResult.IsValid());
      const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
      if (result)
      {
         if (projectSettings.JoinSessionToastTag.IsValid())
         {
            UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForLocalIndex(_travelNextPlayerIdx), projectSettings.JoinSessionToastTag, projectSettings.JoinSessionToastMessage);
         }
      }
      else
      {
         if (projectSettings.JoinSessionFailedToastTag.IsValid())
         {
            UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForLocalIndex(_travelNextPlayerIdx), projectSettings.JoinSessionFailedToastTag, projectSettings.JoinSessionFailedToastMessage);
         }
      }
      return result;
   }  
   return false;
}

bool UTATGameInstance::_TravelToSession()
{
   if (!_travelReadyToTravel)
      return false;
   if (_travelNextPlayerIdx == INDEX_NONE)
      return false;
   if (_travelNextSessionName == NAME_None)
      return false;
   if (ClientTravelToSession(_travelNextPlayerIdx, _travelNextSessionName))
   {
      const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
      if (projectSettings.TravelToSessionToastTag.IsValid())
      {
         UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForLocalIndex(_travelNextPlayerIdx), projectSettings.TravelToSessionToastTag, projectSettings.TravelToSessionToastMessage);
      }
      _TravelResetParameters();
      return true;
   }
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
   if (projectSettings.TravelToSessionFailedToastTag.IsValid())
   {
      UTATUIFunctionLibrary::RequestToastIfLocallyControlled(_GetPlayerControllerForLocalIndex(_travelNextPlayerIdx), projectSettings.TravelToSessionFailedToastTag, projectSettings.TravelToSessionFailedToastMessage);
   }
   return false;
}

void UTATGameInstance::_TravelResetParameters()
{
   _travelNextPlayerIdx = INDEX_NONE;
   _travelNextSessionName = NAME_None;
   _travelSearchResult = FOnlineSessionSearchResult();
}

void UTATGameInstance::_SetNetworkFailure(ENetworkFailure::Type failureType)
{
   // annoyingly there isn't a C++ way to catch these so it's going up
   // to blueprints which we expect to call back into here
   _showNetworkFailure = true;
   _networkFailureType = failureType;
}

void UTATGameInstance::_SetTravelFailure(ETravelFailure::Type failureType)
{
   // annoyingly there isn't a C++ way to catch these so it's going up
   // to blueprints which we expect to call back into here
   _showTravelFailure = true;
   _travelFailureType = failureType;
}

void UTATGameInstance::_InitNetworkVersionOverride()
{
   _netVersion = FEngineVersion::Current().GetChangelist();

#if OSE_CHEATS_ENABLED
   uint32 retVersion = INDEX_NONE;
   FParse::Value(FCommandLine::Get(), TEXT("networkversionoverride="), retVersion);
   if (retVersion != INDEX_NONE)
   {
      _netVersion = retVersion;
   }
#endif

   UE_LOG(LogTATGameInstance, Log, TEXT("TAT Network Version: %d"), _netVersion);

   // give us a chance to tell the engine our version number when it needs it
   FNetworkVersion::GetLocalNetworkVersionOverride.BindLambda(
      [this]()
      {
         return _netVersion;
      }
   );
}

void UTATGameInstance::_OnPostLoadMap(UWorld* world)
{
   OnMapLoadComplete();
   if (!IsValid(_analyticsMgr))
      return;
   _analyticsMgr->OnPostLoadMapWithWorld(world);
}

void UTATGameInstance::_HandlePreClientTravel(const FString& pendingURL, ETravelType travelType, bool isSeamlessTravel)
{
   OnPreClientTravel(isSeamlessTravel);
}

void UTATGameInstance::_InstallMetricsHandlers()
{
   check(_metricsSystem);
   
   TWeakObjectPtr<UTATGameInstance> weakThis = this;
   static const FName tatMetricsGroup = FName("TAT");
   _metricsSystem->AddMetricsGroup(tatMetricsGroup);

   _metricsSystem->AddMetric(
      tatMetricsGroup, FName("MatchSettings"),
      [weakThis](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         UTATGameInstance* self = weakThis.Get();
         if (self == nullptr)
         {
            return nullptr;
         }
         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         const UTATMatchSettingsBase& matchSettings = self->GetMatchSettings();
         TArray<FTATMatchSettingsPropertyDef> propDefs;
         matchSettings.GetAllMatchSettingsProperties(propDefs);
         for (const FTATMatchSettingsPropertyDef& prop : propDefs)
         {
            FString value;
            if (matchSettings.GetMatchSettingsValueAsString(prop.Name, value))
            {
               result->Values.Add(prop.Name.ToString(), MakeShared<FJsonValueString>(MoveTemp(value)));
            }
         }
         return result;
      });
   
   _metricsSystem->AddMetric(
      tatMetricsGroup, FName("GamePhase"),
      [weakThis](const TMap<FString, FString>& queryParams) -> TSharedPtr<FJsonObject>
      {
         UTATGameInstance* self = weakThis.Get();
         if (self == nullptr)
         {
            return nullptr;
         }
         UWorld* world = self->GetWorld();
         if (world == nullptr)
         {
            return nullptr;
         }
         ATATGameState* gameState = world->GetGameState<ATATGameState>();
         if (gameState == nullptr)
         {
            return nullptr;
         }
         TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
         result->Values.Add(TEXT("CurrentPhase"), OSEMetricsUtils::EnumToJsonString(gameState->GetCurrentPhase()));
         result->Values.Add(TEXT("TimeLeftInPhase"), MakeShared<FJsonValueNumber>(gameState->GetTimeLeftInPhase()));
         result->Values.Add(TEXT("CurrentPhaseDuration"), MakeShared<FJsonValueNumber>(gameState->GetCurrentPhaseDuration()));
         result->Values.Add(TEXT("CurrentPhaseStart"), MakeShared<FJsonValueNumber>(gameState->GetCurrentPhaseStart()));
         return result;
      });

   // Rather than duplicating the existing player state array with a parallel array of player data,
   // extend the base one with TAT-specific player data.
   _gameSessionMetrics->ExtendPlayerStateMetrics([](const APlayerState* ps, const TSharedPtr<FJsonObject>& result)
   {
      const ATATPlayerState* playerState = Cast<ATATPlayerState>(ps);
      if (playerState == nullptr)
      {
         return;
      }

      result->Values.Add(TEXT("CharacterType"), OSEMetricsUtils::EnumToJsonString(playerState->GetTATCharacter()));
      result->Values.Add(TEXT("MapIntroState"), OSEMetricsUtils::EnumToJsonString(playerState->GetMapIntroState()));

      TSharedPtr<FJsonObject> activeQuests = MakeShared<FJsonObject>();
      for (int64 i = 0; i < static_cast<int64>(ETATPlayerQuestSlot::MAX); i++)
      {
         const ETATPlayerQuestSlot slot = static_cast<ETATPlayerQuestSlot>(i);
         TSharedPtr<FJsonObject> questSlot = MakeShared<FJsonObject>();
         questSlot->Values.Add(TEXT("QuestTag"), MakeShared<FJsonValueString>(playerState->GetActiveQuestTag(slot).ToString()));
         questSlot->Values.Add(TEXT("ObjectiveComplete"), MakeShared<FJsonValueBoolean>(playerState->IsQuestObjectiveComplete(slot)));
         activeQuests->Values.Add(StaticEnum<ETATPlayerQuestSlot>()->GetNameStringByValue(i), MakeShared<FJsonValueObject>(questSlot));
      }
      result->Values.Add(TEXT("ActiveQuests"), MakeShared<FJsonValueObject>(activeQuests));

      if (const UTATLootInventoryComponent* inventoryComp = playerState->GetLootInventoryComponent())
      {
         result->Values.Add(TEXT("MinorLootCount"), MakeShared<FJsonValueNumber>(inventoryComp->GetMinorLootCount()));
         result->Values.Add(TEXT("MajorLootCount"), MakeShared<FJsonValueNumber>(inventoryComp->GetMajorLootCount()));
         result->Values.Add(TEXT("TotalHeldLootValue"), MakeShared<FJsonValueNumber>(inventoryComp->GetTotalHeldLootValue()));
         result->Values.Add(TEXT("TotalStashedLootValue"), MakeShared<FJsonValueNumber>(inventoryComp->GetTotalStashedLootValue()));
      }
   });
}
