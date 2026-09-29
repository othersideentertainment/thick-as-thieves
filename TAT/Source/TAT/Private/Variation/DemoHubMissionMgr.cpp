// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/DemoHubMissionMgr.h"

// tat
#include "Analytics/TATAnalyticsManager.h"
#include "GameFramework/TATTravelMgr.h"
#include "Matchmaking/Lobby/TATMatchLobbyDummyPlayer.h"
#include "Online/TATGameSession.h"
#include "Online/TATGameState.h"
#include "TATGameInstance.h"
#include "Player/TATPlayerState.h"
#include "Player/TATDemoHubPlayerController.h"
#include "Quests/TATContractSelectionComponent.h"
#include "Quests/TATContractPrioritizer.h"
#include "Developer/TATProjectSettings.h"
#include "Settings/TATMatchSettingsBase.h"
#include "Settings/TATMatchSettings.h"

// ue5
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Misc/UObjectToken.h"
#include "TimerManager.h"

#include "Online/TATHubGameState.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(DemoHubMissionMgr)
DEFINE_LOG_CATEGORY_STATIC(LogTATDemoHubMissionMgr, Log, All);

#if WITH_EDITOR
void ATATDemoHubMissionMgr::CheckForErrors()
{
   Super::CheckForErrors();

#if WITH_EDITORONLY_DATA
   if (!_validatePlayerDummyInstances)
   {
      return;
   }
#endif // WITH_EDITORONLY_DATA

   FFormatNamedArguments arguments;
   arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));

   auto logError = [&](const FString& formatString, const FFormatNamedArguments& inArguments)
      {
         FMessageLog("MapCheck").Error()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(formatString), inArguments)));
      };

   if (!_localPlayerDummy)
   {
      logError(TEXT("{ActorName} : _localPlayerDummy unassigned!"), arguments);
   }
   else if (!_localPlayerDummy->IsForLocalPlayer)
   {
      logError(TEXT("{ActorName} : _localPlayerDummy has IsForLocalPlayer = false!"), arguments);
   }

   const int32 maxNumPlayers = ATATGameSession::StaticClass()->GetDefaultObject<ATATGameSession>()->MaxPlayers;
   if (_remotePlayerDummies.Num() < maxNumPlayers - 1)
   {
      logError(TEXT("{ActorName} : _remotePlayerDummies does not have enough entries to support max player count!"), arguments);
   }
   for (const ATATMatchLobbyDummyPlayer* playerDummy : _remotePlayerDummies)
   {
      if (!playerDummy)
      {
         logError(TEXT("{ActorName} : null entry found in _remotePlayerDummies!"), arguments);
      }
      else if (playerDummy->IsForLocalPlayer)
      {
         arguments.Add(TEXT("DummyName"), FText::FromString(playerDummy->GetPathName()));
         logError(TEXT("{ActorName} : entry {DummyName} in _remotePlayerDummies has IsForLocalPlayer = true!"), arguments);
         arguments.Remove(TEXT("DummyName"));
      }
   }
}
#endif // WITH_EDITOR

ATATDemoHubMissionMgr::ATATDemoHubMissionMgr()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
   bReplicates = true;
   bAlwaysRelevant = true;
}

ATATDemoHubMissionMgr* ATATDemoHubMissionMgr::GetDemoHubMissionManager(const UObject* contextObject)
{
   // This isn't an ideal implementation, but in practice it's used in a very limited context (only during match setup)
   // and only in the demo hub level which has very few actors.
   if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
   {
      for (TActorIterator<ATATDemoHubMissionMgr> it(world); it; ++it)
      {
         if (ATATDemoHubMissionMgr* missionMgr = *it)
         {
            return missionMgr;
         }
      }
   }
   return nullptr;
}

void ATATDemoHubMissionMgr::LeaveLobby()
{
   _ClearMatchmakingTimeout(true);   
}

void ATATDemoHubMissionMgr::_OnTimeoutCooldownComplete()
{
   OnMatchmakingTimeout.Broadcast();
}

void ATATDemoHubMissionMgr::_ClearMatchmakingTimeout(bool clearCooldown)
{
   FTimerManager& timerManager = GetWorld()->GetTimerManager();

   timerManager.ClearTimer(_matchmakingTimeoutHandle);

   if (clearCooldown)
   {
      timerManager.ClearTimer(_matchmakingCooldownHandle);
   }
}

void ATATDemoHubMissionMgr::BeginPlay()
{
   Super::BeginPlay();

   if(HasAuthority())
   {
      const UTATProjectSettings& settings = UTATProjectSettings::Get();
      const UTATGameInstance& gameInstance = UTATGameInstance::Get(GetWorld());
      FTATMapNodeSettings mapNodeSettings = gameInstance.GetMapNodeSettings();

      for (const FTATMapSettings& map : settings.Maps)
      {
         if (map.Map.ToSoftObjectPath() == mapNodeSettings.Map.ToSoftObjectPath())
         {
            AuthoritySetSelectedMap(map.Map, map.GameMode);
         }
      }
   }
}

void ATATDemoHubMissionMgr::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);

   // Wait until local player state is init'd to assign players to dummies
   // TODO: move this to OnLocalPlayerStateAdded once it is fixed to function properly
   if (!IsNetMode(NM_DedicatedServer))
   {
      // Wait for game state so we can rely on player-state-added hooks
      if (ATATGameState* gs = GetWorld()->GetGameState<ATATGameState>())
      {
         if (_localPlayerDummy != nullptr && !_localPlayerDummy->IsClaimed())
         {
            if (const ATATPlayerState* ps = ATATPlayerState::GetLocalTATPlayerState(this))
            {
               _RefreshPlayerDummyAssignments();

               // Subscribe to player state add/remove to refresh dummy representations
               gs->OnPlayerStateAdded.AddDynamic(this, &ATATDemoHubMissionMgr::_OnPlayerStateAdded);
               gs->OnPlayerStateRemoved.AddDynamic(this, &ATATDemoHubMissionMgr::_OnPlayerStateRemoved);
               if (ATATHubGameState* hgs = GetWorld()->GetGameState<ATATHubGameState>())
               {
                  hgs->OnPlayerStateChanged.AddDynamic(this, &ATATDemoHubMissionMgr::_OnPlayerStateChanged);
               }
            }
         }
      }
   }

   // on the server, wait for all players to be inside of the marker
   if (HasAuthority())
   {
      const ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>();
      const float serverWorldTimeNow = GetWorld()->GetTimeSeconds();
      if (!ensure(gameState)) return;

      // all players are ready to go, start the countdown
      float newServerTravelWorldTime = _authorityTravelWorldTime;
      if (gameState->GetAreAllPlayersReadyChecked())
      {
         if (_authorityTravelWorldTime == float(INDEX_NONE))
         {
            newServerTravelWorldTime = serverWorldTimeNow + SecondsToTravel;
         }
      }
      else
      {
         newServerTravelWorldTime = float(INDEX_NONE);
      }

      if (newServerTravelWorldTime != _authorityTravelWorldTime)
      {
         _authorityTravelWorldTime = newServerTravelWorldTime;
         _UpdateHUDMissionCountdown();
      }

      // If the countdown has finished, begin traveling
      if (_authorityTravelWorldTime != float(INDEX_NONE) &&
          serverWorldTimeNow > _authorityTravelWorldTime &&
          !_mapChangeData.SelectedMap.IsNull())
      {
         // check to see if we have already started traveling to the game instance
         if (!_authorityTravelStarted)
         {
            _AuthorityUpdateParty();
            _OnAuthorityTravelToMap();
         }
      }
   }
}

void ATATDemoHubMissionMgr::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ATATDemoHubMissionMgr, _authorityTravelWorldTime);
   DOREPLIFETIME(ATATDemoHubMissionMgr, _mapChangeData);
}

void ATATDemoHubMissionMgr::_OnRep_ServerTravelWorldTime()
{
   _UpdateHUDMissionCountdown();
}

void ATATDemoHubMissionMgr::_UpdateHUDMissionCountdown()
{
   OnTravelWorldTimeChanged.Broadcast(_authorityTravelWorldTime);
}

void ATATDemoHubMissionMgr::_AuthorityUpdateParty()
{
   check(HasAuthority());
   const ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>();
   UTATGameInstance* gameInstance = GetWorld()->GetGameInstance<UTATGameInstance>();
   check(gameState);
   check(gameInstance);

   TArray<FTATContractWithNetId> partyQuests;

   // Tell the game instance this is our new player group (party)
   gameInstance->ClearParty();
   for (const TObjectPtr<APlayerState>& playerState : gameState->PlayerArray)
   {
      if (playerState != nullptr)
      {
         gameInstance->AddPlayerToParty(playerState->GetUniqueId());

         if (const UTATContractSelectionComponent* questSelection = playerState->FindComponentByClass<UTATContractSelectionComponent>())
         {
            FGameplayTag questTag = questSelection->GetSelectedContract();
            if (questTag.IsValid())
            {
               partyQuests.Add({questTag, playerState->GetUniqueId() });
            }
         }
      }
   }
   // choose the contract based on selected contract of players
   gameInstance->SetContractSelections(TATContractPrioritizer::BuildPartyContractsFromIndividuals(this, _mapChangeData.SelectedMap, partyQuests));
}

void ATATDemoHubMissionMgr::_RefreshPlayerDummyAssignments()
{
   check(!IsNetMode(NM_DedicatedServer));

   // Should only be called when local player state is init'd and game state is present
   const ATATGameState* gameState = GetWorld()->GetGameStateChecked<ATATGameState>();
   const TArray<ATATPlayerState*>& playerStateArray = gameState->GetTATPlayerStates();
   
   // Assign local player to their decidated dummy
   UE_LOG(LogTATDemoHubMissionMgr, Verbose, TEXT("_AssignPlayersToDummies() | Reassiging local player + %d opponents to dummies...")
      , playerStateArray.Num() - 1);
   if (_localPlayerDummy)
   {
      ATATPlayerState* localPlayer = ATATPlayerState::GetLocalTATPlayerState(this);
      if (ensure(localPlayer) && _localPlayerDummy->GetPlayer() != localPlayer)
      {
         _localPlayerDummy->AssignPlayer(localPlayer);
      }
   }
   else
   {
      UE_LOG(LogTATDemoHubMissionMgr, Error, TEXT("_localPlayerDummy is unassigned!"));
   }

   // Clear all remote player dummies
   for (ATATMatchLobbyDummyPlayer* remoteDummy : _remotePlayerDummies)
   {
      if (ensureMsgf(remoteDummy, TEXT("Invalid entry found in _remotePlayerDummies!")))
      {
         remoteDummy->ClearPlayer();
      }
   }

   // Reassign remote players to dummies
   UE_CLOG(_remotePlayerDummies.Num() < playerStateArray.Num() - 1, LogTATDemoHubMissionMgr, Error, TEXT("%d remote players detected, but only %d dummies present in _remotePlayerDummies!")
      , playerStateArray.Num() - 1
      , _remotePlayerDummies.Num());
   for (ATATPlayerState* playerState : playerStateArray)
   {
      if (!playerState->IsInactive() && !playerState->IsLocalPlayerState())
      {
         ATATMatchLobbyDummyPlayer* remoteDummy = _FindUnclaimedDummyForRemotePlayer();
         if (ensure(remoteDummy))
         {
            remoteDummy->AssignPlayer(playerState);
         }
      }
   }

   OnRefreshDummyAssignments.Broadcast();
}

ATATMatchLobbyDummyPlayer* ATATDemoHubMissionMgr::_FindUnclaimedDummyForRemotePlayer() const
{
   for (ATATMatchLobbyDummyPlayer* remoteDummy : _remotePlayerDummies)
   {
      if (!remoteDummy->IsClaimed())
      {
         return remoteDummy;
      }
   }
   return nullptr;
}

void ATATDemoHubMissionMgr::_OnAuthorityTravelToMap()
{
   _authorityTravelStarted = true;
   FURL url(nullptr, TEXT(""), TRAVEL_Absolute);
   url.Map = _mapChangeData.SelectedMap.GetAssetName();
   if(_mapChangeData.GameMode.IsValid())
   {
      url.AddOption(*FString::Printf(TEXT("GAME=%s"), *_mapChangeData.GameMode.ToString()));
   }
   UTATTravelMgr::ServerInitiateTravel(this, url.ToString(), ETATTravelType::WithLoadingScreen);
}

void ATATDemoHubMissionMgr::AuthoritySetSelectedMap(const TSoftObjectPtr<UWorld>& newMap, const FSoftClassPath& gameMode)
{
   check(HasAuthority());

   UTATGameInstance* gameInstance = Cast<UTATGameInstance>(GetGameInstance());
   if (gameInstance)
   {
      const FTATMatchSettingsQueryContext queryContext = FTATMatchSettingsQueryContext(newMap, this);
      gameInstance->GetMatchSettings().InitMatchSettingsForNewMatch(queryContext);

      // Inject difficulty and selected mission into replicated data along with map
      const ETATDifficulty difficulty = gameInstance->GetMapNodeSettings().Difficulty;
      _mapChangeData.Difficulty = difficulty;
      if (UTATMatchSettings* matchSettings = UTATMatchSettingsBase::GetMutableMatchSettings<UTATMatchSettings>(this))
      {
         _mapChangeData.MissionTag = matchSettings->Mission;

         // Also set difficulty in match settings, since that is what will be used in the match
         matchSettings->Difficulty = difficulty;
      }
   }

   _mapChangeData.SelectedMap = newMap;
   _mapChangeData.GameMode = gameMode;

   const bool forcePvp = FParse::Param(FCommandLine::Get(), TEXT("pvp"));
   if (forcePvp)
   {
      _mapChangeData.GameMode = UTATProjectSettings::Get().PvpGameMode;
   }

   OnSelectedMapChanged.Broadcast();
}

void ATATDemoHubMissionMgr::_OnPlayerStateAdded(AOSEPlayerState* playerState)
{
   _RefreshPlayerDummyAssignments();
}

void ATATDemoHubMissionMgr::_OnPlayerStateRemoved(AOSEPlayerState* playerState)
{
#if WITH_EDITOR
   // On PIE listen server, if the host ends the session their player state will be removed from 
   // PlayerStateArray before the clients, which leads to a failed assert on the presence of a
   // local-player state in _RefreshPlayerDummyAssignments() when the clients are removed.
   // 
   // The absence/removal of a local player state here serves as a heuristic for world-teardown, 
   // so just skip the work of refreshing player dummies.
   if (GetNetMode() == NM_ListenServer)
   {
      const AOSEPlayerState* ps = AOSEPlayerState::GetLocalOSEPlayerState(this);
      if (!ps || ps == playerState)
      {
         return;
      }
   }
#endif // WITH_EDITOR

   // If anyone left the lobby, unready ourselves so that the condition of "All players in room are ready" aren't met the moment someone leaves.
   if (ATATPlayerState* localPlayer = ATATPlayerState::GetLocalTATPlayerState(this))
   {
      localPlayer->ServerSetIsReadyChecked(false);
   }
   _RefreshPlayerDummyAssignments();
}

void ATATDemoHubMissionMgr::_OnPlayerStateChanged(AOSEPlayerState* playerState)
{
   _RefreshPlayerDummyAssignments();
}

void ATATDemoHubMissionMgr::_OnRep_MapChangeData()
{
   OnSelectedMapChanged.Broadcast();
}
