// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATGameState.h"

// tat
#include "TATGameInstance.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Variation/TATSpawnTiming.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "Online/TATPvPGameMode.h"
#include "Developer/TATEditorSettings.h"
#include "Quests/TATActiveQuestSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "Settings/TATMatchSettingsBase.h"
#include "UI/TATCompassPOIWidget.h"
#include "UI/TATCompassWidget.h"
#include "WorldMap/TATWorldMapSubsystem.h"
#include "Analytics/TATLocalPlayerAnalyticsSubsystem.h"

// ose
#include "Graphics/Performance/OSEPerformanceTestComponent.h"

// ue4
#include "Net/UnrealNetwork.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameState)

DEFINE_LOG_CATEGORY_STATIC(LogTATGameState, Log, All);

namespace GameStateHelpers
{
   // could have used USOECommon, but :shrug: (or made the enum values be a bitmask directly, but I didn't really want to have a none)
   // Not hard to change later
   static uint8 ReasonToMask(ETATEndgameReason reason)
   {
      return 1 << static_cast<uint8>(reason);
   }

   static FString GetSessionName(const UWorld* world)
   {
      const IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(world);
      if (sessionInterface.IsValid())
      {
         if (const FNamedOnlineSession* session = sessionInterface->GetNamedSession(NAME_GameSession))
         {
            return session->OwningUserName;
         }
      }

      return FString();
   }

   static FString GetVariantsSummary(const ATATGameState& gameState)
   {
      if (const UTATMapVariationMgrComponent* variationMgr = gameState.GetMapVariationMgr())
      {
         return variationMgr->GetActiveVariants().ToCompactString();
      }
      return FString();
   }

   static void SetCrashReporterProperties(const ATATGameState& gameState)
   {
      // Map is set in TravelMgr
      FGenericCrashContext::SetGameData(TEXT("Session"), GetSessionName(gameState.GetWorld()));
      FGenericCrashContext::SetGameData(TEXT("MapSeed"), FString::FromInt(gameState.GetMapSeedUnchecked()));
      FGenericCrashContext::SetGameData(TEXT("NetMode"), ToString(gameState.GetNetMode()));
      FGenericCrashContext::SetGameData(TEXT("ActiveVariants"), GetVariantsSummary(gameState));
   }
}

/* static */
ATATGameState* ATATGameState::GetTATGameState(const UObject* contextObj)
{
   check(contextObj);
   return contextObj->GetWorld()->GetGameState<ATATGameState>();
}

/* static */
ATATGameState* ATATGameState::Get(const UObject& contextObj)
{
   return GetTATGameState(&contextObj);
}

ATATGameState::ATATGameState(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   _mapVariationMgr = CreateOptionalDefaultSubobject<UTATMapVariationMgrComponent>(TEXT("MapVariationMgr"));
   if (_mapVariationMgr)
   {
      _mapVariationMgr->OnVariantsReplicated.BindUObject(this, &ATATGameState::_CheckForReplicatedBeginPlay);
   }
}

void ATATGameState::BeginPlay()
{
   Super::BeginPlay();

   // the server is going to listen to player controllers coming and going so that we can listen
   // to characters coming and going so that we can bind to character-based events and do some bookkeeping.
   if (HasAuthority())
   {
      // add any player controllers that already exist
      for (auto it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
      {
         if (ATATPlayerController* playerController = Cast<ATATPlayerController>(it->Get()))
         {
            _AuthorityOnPlayerControllerAdded(GetWorld()->GetAuthGameMode(), playerController);
         }
      }

      // bind for player controllers that come and go
      FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &ATATGameState::_AuthorityOnPlayerControllerAdded);
      FGameModeEvents::OnGameModeLogoutEvent().AddUObject(this, &ATATGameState::_AuthorityOnPlayerControllerRemoved);

      // Set up the end-of-match timer
      if (_CanEverStartMatch())
      {
         // How often to check if we're ready to schedule the end of the match
         constexpr float tryScheduleMatchEndTimerFrequencySeconds = 0.2f;

         AuthorityCalculateMatchDuration();
         _tryStartMatchStartTime = GetWorld()->GetTimeSeconds();
         GetWorld()->GetTimerManager().SetTimer(
            _tryStartMatchTimer,
            FTimerDelegate::CreateUObject(this, &ThisClass::_TryStartMatch),
            tryScheduleMatchEndTimerFrequencySeconds,
            true,
            0.0f);
      }
   }
   
   if (ATATWorldSettings::Get(this).MapType == ETATMapType::Mission)
   {
      if (const ULocalPlayer* player = GetWorld()->GetFirstLocalPlayerFromController())
      {
         if (UTATLocalPlayerAnalyticsSubsystem* analyticsSubsystem = player->GetSubsystem<UTATLocalPlayerAnalyticsSubsystem>())
         {
            analyticsSubsystem->HandleMatchStart();
         }
      }
   }

#if WITH_EDITOR && OSE_CHEATS_ENABLED
   if (GEngine->IsEditor())
   {
      GetWorldTimerManager().SetTimerForNextTick(this, &ATATGameState::_OnAutoExecCheats);
   }
#endif

   GameStateHelpers::SetCrashReporterProperties(*this);
}

bool ATATGameState::IsMissionOwner(APlayerState* playerState) const
{
   if (playerState == nullptr || _missionOwnerPlayerState == nullptr)
   {
      return false;
   }
   return playerState->GetUniqueId() == _missionOwnerPlayerState->GetUniqueId();
}

void ATATGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ATATGameState, _matchSettingsData);
   DOREPLIFETIME(ATATGameState, _isGameFrozenCounter);
   DOREPLIFETIME(ATATGameState, _missionOwnerPlayerState);
   DOREPLIFETIME(ATATGameState, _matchPhase);
   DOREPLIFETIME(ATATGameState, _playerHasEscapedWithMatchObjective);
   DOREPLIFETIME(ATATGameState, _matchDuration);
   DOREPLIFETIME_CONDITION(ATATGameState, _mapSeed, COND_InitialOnly);
}

void ATATGameState::HandleBeginPlay()
{
   // only called from game mode
   // Might not be present in some subclasses (e.g. hub)
   if (_mapVariationMgr)
   {
      _mapVariationMgr->AuthorityChooseSceneVariants();
   }

   // Init our copy of the match settings data to whatever the game instance currently has so it can be replicated to clients
   // (We don't have to keep this up to date because we're listening for the OnMatchSettingsUpdated event which will handle that)
   UTATGameInstance::Get(this).GetMatchSettings().SerializeToByteArray(_matchSettingsData);

   Super::HandleBeginPlay();
}

void ATATGameState::ServerResetAllPlayerReadyChecks()
{
   if (HasAuthority())
   {
      for (ATATPlayerState* ps : GetTATPlayerStates())
      {
         ps->ServerSetIsReadyChecked(false);
      }
   }
}

void ATATGameState::GetNumPlayersReadyChecked(int& readyChecked, int& totalPlayers) const
{
   const TArray<ATATPlayerState*>& playerStates = GetTATPlayerStates();

   readyChecked = 0;
   totalPlayers = playerStates.Num();
   for (ATATPlayerState* ps : GetTATPlayerStates())
   {
      if (ps->IsReadyChecked())
         readyChecked++;
   }
}

bool ATATGameState::GetAreAllPlayersReadyChecked() const
{
   int readyChecked = 0;
   int numPlayers = 0;
   GetNumPlayersReadyChecked(readyChecked, numPlayers);
   return numPlayers > 0 && numPlayers == readyChecked;
}

void ATATGameState::AuthorityResetAllPlayersReadyForCutsceneSkip()
{
   if (HasAuthority())
   {
      for (ATATPlayerState* ps : GetTATPlayerStates())
      {
         ps->ServerSetIsReadyForCutsceneSkip(false);
      }
   }
}

void ATATGameState::GetNumPlayersReadyForCutsceneSkip(int& readyToSkipCutscene, int& totalPlayers) const
{
   const TArray<ATATPlayerState*>& playerStates = GetTATPlayerStates();

   readyToSkipCutscene = 0;
   totalPlayers = playerStates.Num();
   for (ATATPlayerState* ps : GetTATPlayerStates())
   {
      if (ps->IsReadyForCutsceneSkip())
         readyToSkipCutscene++;
   }
}

bool ATATGameState::GetAreAllPlayersReadyForCutsceneSkip() const
{
   int readyToSkipCutscene = 0;
   int numPlayers = 0;
   GetNumPlayersReadyForCutsceneSkip(readyToSkipCutscene, numPlayers);
   return numPlayers > 0 && numPlayers == readyToSkipCutscene;
}

void ATATGameState::AuthoritySetGameFrozen(bool newFrozen)
{
   if (HasAuthority())
   {
      bool wasFrozen = IsGameFrozen();
      if (newFrozen)
         _isGameFrozenCounter++;
      else
         _isGameFrozenCounter = FMath::Max(0, --_isGameFrozenCounter);
      bool isFrozen = IsGameFrozen();
      if (wasFrozen != isFrozen)
      {
         _BroadcastIsGameFrozenChanged();
      }
   }
}

void ATATGameState::_AuthorityUpdateMissionOwnerPlayerState()
{
   // update the mission owner player state
   if (HasAuthority())
   {
      // ASSUMPTION: Player controller 0 is the host, this will be true in p2p but may not
      // be true when we're on dedicated servers w/ matchmaking so we'll want to adjust in the future
      // NOTE: this logic can be pretty flakey in PIE as the two windows often race to load so we may
      // want to test this sort of "mission owner" stuff on a listen server while this (temp) logic exists
      ATATPlayerController* missionOwnerPC = ATATPlayerController::GetTATPlayerController(this, 0);
      ATATPlayerState* missionOwnerPS = missionOwnerPC ? missionOwnerPC->GetTATPlayerState() : nullptr;

      if (_missionOwnerPlayerState != missionOwnerPS)
      {
         _missionOwnerPlayerState = missionOwnerPS;
         _BroadcastMissionOwnerChanged();
      }
   }
}

void ATATGameState::_TryStartMatch()
{
   check(HasAuthority());
   check(_tryStartMatchStartTime >= 0);

   // Schedule the match end when all players are ready (i.e. character ready + completed map intro), or after waiting a few seconds, whichever comes first.
   const double secondsWaited = GetWorld()->GetTimeSeconds() - _tryStartMatchStartTime;
   if (_maxTimeInSecondsBeforeStartingMatchTimer > 0 && secondsWaited < _maxTimeInSecondsBeforeStartingMatchTimer)
   {
      // Get list of party members expected to join
      const UTATGameInstance* tatGameInstance = GetGameInstance<UTATGameInstance>();
      check(tatGameInstance);
      const TArray<FUniqueNetIdRepl>& partyMembers = tatGameInstance->GetPartyMembers();

      // Wait until each party member has reconnected after the server travel
      // NOTE: only necessary for non-seamless server travel, which makes clients disconnect -> reconnect
      const AGameStateBase* gs = GetWorld()->GetGameState();
      check(gs);
      for (const FUniqueNetIdRepl& playerId : partyMembers)
      {
         const APlayerState* ps = gs->GetPlayerStateFromUniqueNetId(playerId);
         if (!ps)
         {
            return;
         }
      }

      // Wait until all connected players have completed the map intro sequence
      for (const ATATPlayerState* ps : GetTATPlayerStates())
      {
         if (ps && ps->GetMapIntroState() != ETATMapIntroState::Complete)
         {
            return;
         }
      }
   }
   UE_CLOG(_maxTimeInSecondsBeforeStartingMatchTimer > 0 && secondsWaited >= _maxTimeInSecondsBeforeStartingMatchTimer,
      LogTATGameState, Warning, TEXT("Started match without waiting for players after %f seconds"), secondsWaited);

   float mainPhaseDurationInSeconds;
   UWorld* world = GetWorld();
   // Set main phase duration to indefinite when in FTUE
   if (ATATWorldSettings::Get(this).MapType == ETATMapType::Tutorial)
   {
      mainPhaseDurationInSeconds = 0.0f;
   }
   else
   {
      float totalMatchDuration = GetTotalMatchDuration();
      if (totalMatchDuration > 0.0f)
      {
         const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(world);
         float endgameDuration = UTATProjectSettings::Get().GetEndgameDurationForDifficulty(currentDifficulty);
         if (totalMatchDuration > endgameDuration)
         {
            mainPhaseDurationInSeconds = totalMatchDuration - endgameDuration;
         }
         else
         {
            // If the intended Total Match Duration is less than the Endgame Duration, just start the Endgame right away by setting the duration of Main Phase to 1 second.
            // Note: We don't set it to 0 seconds because that indicates indefinite Main Phase time.
            mainPhaseDurationInSeconds = 1.0f;
         }
      }
      else
      {
         // A total match duration of 0 seconds means we want an indefinite Main Phase.
         mainPhaseDurationInSeconds = 0.0f;
      }
   }

   // Start the match by starting the main phase
   _AuthorityStartPhase(ETATMatchPhase::Main, mainPhaseDurationInSeconds);

   // start fallback timer as well
   // This ensures that the endgame timer eventually starts even after entering the endgame without a timer
   world->GetTimerManager().SetTimer(_fallbackMatchTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::_OnFallbackMatchTimerEnd), mainPhaseDurationInSeconds, false);
}

void ATATGameState::_OnAutoExecCheats()
{
#if WITH_EDITOR && OSE_CHEATS_ENABLED
   UWorld* world = GetWorld();
   check(world != nullptr);
   for (const FTATEditorSettingsCheatCommand& cmd : UTATEditorSettings::Get().AutoExecCheats)
   {
      if (cmd.Context != ETATEditorSettingsCheatCommandContext::Player)
      {
         cmd.RunCheat(world, nullptr);
      }
   }
#endif
}

float ATATGameState::GetTimeLeftInPhase() const
{
   return FMath::Max(0.0f, _matchPhase.EndAt - GetServerWorldTimeSeconds());
}

float ATATGameState::GetTotalMatchDuration() const
{
   return _matchDuration;
}

float ATATGameState::GetRemainingMatchDuration() const
{
   if (_matchPhase.Phase == ETATMatchPhase::Endgame)
   {
      return GetTimeLeftInPhase();
   }

   if (_matchPhase.Phase == ETATMatchPhase::Main)
   {
      const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
      return GetTimeLeftInPhase() + UTATProjectSettings::Get().GetEndgameDurationForDifficulty(currentDifficulty);
   }

   return GetTotalMatchDuration();
}

void ATATGameState::AuthorityCalculateMatchDuration()
{
   if (UWorld* world = GetWorld())
   {
      if (const UTATActiveQuestSubsystem* activeQuestSubsystem = world->GetSubsystem<UTATActiveQuestSubsystem>())
      {
         if (const FTATMissionInfo* mission = activeQuestSubsystem->GetMissionInfo())
         {
            const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(world);
            _matchDuration = mission->GetMatchDurationForDifficulty(currentDifficulty);
         }
      }
   }
}

bool ATATGameState::AuthorityHasEndGameReason(ETATEndgameReason reason) const
{
   check(HasAuthority());
   return (_authorityEndgameReasonMask & GameStateHelpers::ReasonToMask(reason)) != 0;
}

bool ATATGameState::HasTATMatchStarted() const
{
   return _matchPhase.Phase != ETATMatchPhase::Unstarted;
}

void ATATGameState::CallOrRegisterMatchStartDelegate(const FSimpleMulticastDelegate::FDelegate& startDelegate)
{
   if (HasTATMatchStarted())
   {
      startDelegate.Execute();
   }
   else
   {
      _onMatchStartDelegate.Add(startDelegate);
   }
}

void ATATGameState::AuthorityStartEndgame(ETATEndgameReason reason)
{
   check(HasAuthority());
   if (GetCurrentPhase() >= ETATMatchPhase::Endgame)
   {
      return;
   }

   AuthorityStartEndgameWithDuration(reason, _GetEndgameDurationSeconds(reason));
}

float ATATGameState::_GetEndgameDurationSeconds_Implementation(ETATEndgameReason reason) const
{
   const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
   return UTATProjectSettings::Get().GetEndgameDurationForDifficulty(currentDifficulty);
}

void ATATGameState::AuthorityStartEndgameWithDuration(ETATEndgameReason reason, float duration)
{
   check(HasAuthority());
   if (GetCurrentPhase() < ETATMatchPhase::Endgame)
   {
      _AuthorityStartPhase(ETATMatchPhase::Endgame, duration);
   }

   if (!AuthorityHasEndGameReason(reason))
   {
      _authorityEndgameReasonMask |= GameStateHelpers::ReasonToMask(reason);
      _AuthorityOnEndgameReasonAdded(reason);
   }
}

bool ATATGameState::AuthoritySetPhaseTimeRemaining(float timeRemaining)
{
   check(HasAuthority());

   if(_matchPhase.Duration == 0)
   {
      return false;
   }

   timeRemaining = FMath::Clamp(timeRemaining, 0, _matchPhase.Duration);

   FTATReplicatedMatchPhase newPhase = _matchPhase;
   newPhase.EndAt = GetServerWorldTimeSeconds() + timeRemaining;
   return _AuthoritySetPhase(newPhase);
}

void ATATGameState::AuthorityOnPlayerEscapedWithMatchObjective()
{
   check(HasAuthority());
   // if there is another place we want to do something equivalent, extract a method
   // It is just slightly different enough from the fallback timer (notably starting the endgame, although that is moot there)
   if (_matchPhase.Phase == ETATMatchPhase::Endgame && !_matchPhase.HasTimer())
   {
      // If an end-game timer isn't running yet, start one
      // For now, assuming we don't want to change the timer if already started, but not hard to change that to make it shorten the time
      const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
      _AuthorityStartPhase(ETATMatchPhase::Endgame, UTATProjectSettings::Get().GetEndgameDurationForDifficulty(currentDifficulty));
   }

   if (!_playerHasEscapedWithMatchObjective)
   {
      _playerHasEscapedWithMatchObjective = true;
      _OnRep_PlayerHasEscapedWithMatchObjective(false);
   }
}

void ATATGameState::AuthorityInitializeMapSeed(int32 mapSeed)
{
   check(HasAuthority());
   check(_mapSeed == 0);

   // since using 0 as a sentinel, change it to one if actually 0
   const int32 newMapSeed = mapSeed ? mapSeed : 1;
   _mapSeed = newMapSeed;
}

int32 ATATGameState::GetMapSeed() const
{
#if DO_CHECK
   const ETATMapType mapType = ATATWorldSettings::Get(this).MapType;
   ensure(_mapSeed || mapType == ETATMapType::Menu || mapType == ETATMapType::ThievesDen);
#endif
   return _mapSeed;
}

void ATATGameState::AuthorityNotifyMatchSettingsDataUpdated(TArray<uint8>&& newMatchSettingsData)
{
   if (_matchSettingsData == newMatchSettingsData)
   {
      UE_LOG(LogTATGameState, Verbose, TEXT("AuthorityNotifyMatchSettingsDataUpdated: Got update data that's identical to the old data!"));
      return;
   }

   _matchSettingsData = MoveTemp(newMatchSettingsData);
   ForceNetUpdate();
}

void ATATGameState::_OnOSEPlayerStateAdded(AOSEPlayerState* playerState)
{
   Super::_OnOSEPlayerStateAdded(playerState);
   _UpdateTATPlayerStateArray();
   _AuthorityUpdateMissionOwnerPlayerState();
}

void ATATGameState::_OnOSEPlayerStateRemoved(AOSEPlayerState* playerState)
{
   Super::_OnOSEPlayerStateRemoved(playerState);
   _UpdateTATPlayerStateArray();
   _AuthorityUpdateMissionOwnerPlayerState();
}

void ATATGameState::OnRep_ReplicatedHasBegunPlay()
{
   // NOTE: explicitly not calling super in order to check multiple things
   _CheckForReplicatedBeginPlay();
}

void ATATGameState::_CheckForReplicatedBeginPlay()
{
   // Only begin play if both flag and variants have replicated
   // This might be slightly overkill (since it ought to be in the same bunch), but better safe than sorry
   const bool hasEnoughVariants = _mapVariationMgr == nullptr || _mapVariationMgr->AreVariantsInitialized();
   if (bReplicatedHasBegunPlay && !GetWorld()->GetBegunPlay() && hasEnoughVariants && _hasReplicatedMatchSettings && GetLocalRole() != ROLE_Authority)
   {
      UE_LOG(LogTATGameState, Log, TEXT("Starting play with seed %d"), _mapSeed);
      GetWorldSettings()->NotifyBeginPlay();
      GetWorldSettings()->NotifyMatchStarted();
   }
}

bool ATATGameState::_AuthorityStartPhase(ETATMatchPhase phase, float duration)
{
   check(HasAuthority());

   FTATReplicatedMatchPhase newPhase = {
      .Phase = phase,
      .Duration = duration,
   };

   if(duration > 0)
   {
      newPhase.EndAt = GetWorld()->GetTimeSeconds() + duration;
   }

   UE_LOG(LogTATGameState, Log, TEXT("Starting phase %s with duration %f"),
      *WriteToString<64>(StaticEnum<ETATMatchPhase>()->GetNameByValue(static_cast<int64>(newPhase.Phase))),
      duration);
   return _AuthoritySetPhase(newPhase);
}

bool ATATGameState::_AuthoritySetPhase(const FTATReplicatedMatchPhase& newPhase)
{
   check(HasAuthority());
   if (_matchPhase == newPhase || newPhase.Phase < _matchPhase.Phase)
   {
      return false;
   }

   const FTATReplicatedMatchPhase oldPhase = _matchPhase;
   _matchPhase = newPhase;

   _BroadcastPhaseChanged(oldPhase);

   FTimerManager& timerMgr = GetWorld()->GetTimerManager();

   timerMgr.ClearTimer(_phaseTimerHandle);
   if (_matchPhase.HasTimer())
   {
      const float secondsUntilEnd = _matchPhase.EndAt - GetWorld()->GetTimeSeconds();
      if (secondsUntilEnd > 0)
      {
         timerMgr.SetTimer(_phaseTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::_OnPhaseTimerEnd, _matchPhase.Phase), secondsUntilEnd, false);
      }
      else
      {
         // This is slightly riskier since there is no timer handle to cancel
         timerMgr.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::_OnPhaseTimerEnd, _matchPhase.Phase));
      }
   }

   // The match has been started, no need to keep calling this function
   timerMgr.ClearTimer(_tryStartMatchTimer);
   _tryStartMatchStartTime = -1;

   return true;
}

void ATATGameState::_OnPhaseTimerEnd(ETATMatchPhase phase)
{
   if (!ensure(phase == _matchPhase.Phase))
   {
      return;
   }

   switch(_matchPhase.Phase)
   {
   case ETATMatchPhase::Main:
      AuthorityStartEndgame(ETATEndgameReason::Timer);
      break;
   case ETATMatchPhase::Endgame:
      if (ATATSessionGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATSessionGameMode>())
      {
         gameMode->ForceRemainingPlayersCaught();
      }
      break;
   default:
      // nothing special
      break;
   }
}

void ATATGameState::_OnFallbackMatchTimerEnd()
{
   if (_matchPhase.Phase == ETATMatchPhase::Main || (_matchPhase.Phase == ETATMatchPhase::Endgame && !_matchPhase.HasTimer()))
   {
      // If an end-game timer isn't running yet, start one (including if in endgame with no timer)
      const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
      _AuthorityStartPhase(ETATMatchPhase::Endgame, UTATProjectSettings::Get().GetEndgameDurationForDifficulty(currentDifficulty));
   }
}

void ATATGameState::_AuthorityOnEndgameReasonAdded(ETATEndgameReason reason)
{
   OnAuthorityEndgameReasonAdded.Broadcast(reason);

   if (reason == ETATEndgameReason::Mission)
   {
      _mapVariationMgr->AuthorityExecuteSpawnTiming(ETATSpawnTiming::EndgameQuest);
   }
}

void ATATGameState::_OnRep_PlayerHasEscapedWithMatchObjective(bool previous)
{
   if (_playerHasEscapedWithMatchObjective && !previous)
   {
      OnPlayerEscapedWithMatchObjective.Broadcast();
   }
}

bool ATATGameState::_CanEverStartMatch() const
{
   if(GetWorld()->GetAuthGameMode<ATATSessionGameMode>() == nullptr)
   {
      return false;
   }

   const ETATMapType mapType = ATATWorldSettings::Get(this).MapType;
   if (mapType != ETATMapType::Mission && mapType != ETATMapType::Tutorial)
   {
      return false;
   }

#if OSE_ALLOW_PERFTEST
   // Don't start the timer if running a perf test
   // TODO: if the scheduling becomes dependent on something that is already suppressed by perftest (like player ready), then this can be removed
   if (UOSEPerformanceTestComponent::IsRunningPerformanceTest())
   {
      return false;
   }
#endif

   return true;
}

void ATATGameState::_UpdateTATPlayerStateArray()
{
   // this array is pre-sorted for us
   const TArray<AOSEPlayerState*>& osePlayerStates = GetOSEPlayerStates();
   _tatPlayerStates.Reset();
   for(AOSEPlayerState* osePlayerState : osePlayerStates)
   {
      // should never have null in there, and should always cast correctly, so check it!
      ATATPlayerState* tatPS = CastChecked<ATATPlayerState>(osePlayerState);
      _tatPlayerStates.Add(tatPS);
   }
}

void ATATGameState::_AuthorityOnPlayerControllerAdded(AGameModeBase* gameMode, APlayerController* pc)
{
   if (ATATPlayerController* tatPC = Cast<ATATPlayerController>(pc))
   {
      _AuthorityUpdateMissionOwnerPlayerState();
   }
}

void ATATGameState::_AuthorityOnPlayerControllerRemoved(AGameModeBase* gameMode, AController* pc)
{
   if (ATATPlayerController* tatPC = Cast<ATATPlayerController>(pc))
   {
      tatPC->OnPawnChanged.RemoveAll(this);

      _AuthorityUpdateMissionOwnerPlayerState();
   }
}

void ATATGameState::_OnRep_MatchSettings()
{
   check(!HasAuthority());
   constexpr bool autoUpdateGameState = false; //NB. We are the game state - setting this to true could cause an infinite loop
   UTATGameInstance::Get(this).UpdateMatchSettings(_matchSettingsData, autoUpdateGameState);
   _hasReplicatedMatchSettings = true;
   _CheckForReplicatedBeginPlay();
}

void ATATGameState::_OnRep_IsGameFrozen(int prevCount)
{
   // if either of these are zero we went from off-to-on or vice-versa
   if (prevCount == 0 || _isGameFrozenCounter == 0)
   {
      _BroadcastIsGameFrozenChanged();
   }
}

void ATATGameState::_BroadcastIsGameFrozenChanged()
{
   UE_LOG(LogTATGameState, Verbose, TEXT("Game is %s"), _isGameFrozenCounter ? TEXT("frozen") : TEXT("NOT frozen"));
   OnGameFrozenChanged.Broadcast(IsGameFrozen());
}

void ATATGameState::_OnRep_MatchPhase(const FTATReplicatedMatchPhase& prevPhase)
{
   _BroadcastPhaseChanged(prevPhase);
}

void ATATGameState::_BroadcastPhaseChanged(const FTATReplicatedMatchPhase& prevPhase)
{
   if (prevPhase == _matchPhase)
   {
      return;
   }

   OnPhaseTimerChanged.Broadcast(_matchPhase.Phase);

   if (_matchPhase.Phase != prevPhase.Phase)
   {
      OnPhaseChanged.Broadcast(_matchPhase.Phase, prevPhase.Phase);
   }

   if (HasTATMatchStarted())
   {
      _onMatchStartDelegate.Broadcast();
      _onMatchStartDelegate.Clear();
   }

   if (_matchPhase.Phase == ETATMatchPhase::Endgame)
   {
      // Remove Secondary Objectives from the compass (primarily clues for now)
      if (UTATCompassWidget* compass = UTATCompassWidget::TryGetCompass(this))
      {
         compass->AddCategoryToOmittedPOIs(EPOICategory::SecondaryObjective);
      }

     if (HasAuthority())
     {
        _mapVariationMgr->AuthorityExecuteSpawnTiming(ETATSpawnTiming::EndgameAny);
     }
   }
}

void ATATGameState::_OnRep_MissionOwner()
{
   _BroadcastMissionOwnerChanged();
}

void ATATGameState::_BroadcastMissionOwnerChanged()
{
   UE_LOG(LogTATGameState, Verbose, TEXT("Mission owner is now %s"), _missionOwnerPlayerState ? *_missionOwnerPlayerState->GetPlayerName() : TEXT("NULL"));
   OnMissionOwnerPlayerStateChanged.Broadcast(_missionOwnerPlayerState);
}

