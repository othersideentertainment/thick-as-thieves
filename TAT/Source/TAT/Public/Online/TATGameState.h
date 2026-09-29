// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/TATEndgameReason.h"

// ose
#include "Online/OSEGameState.h"

// ue4
#include "GameplayTagContainer.h"
#include "GameFramework/GameStateBase.h"

#include "TATGameState.generated.h"

class AOSEPlayerState;
class ATATPlayerController;
class ATATPlayerState;
class UTATMapVariationMgrComponent;
class UTATMatchSettingsBase;

UENUM(BlueprintType)
enum class ETATMatchPhase : uint8
{
   Unstarted,
   Main, //< The normal stuff
   Endgame, //< When escapes happen and such
   //Complete //< Maybe Later
};

USTRUCT()
struct FTATReplicatedMatchPhase
{
   GENERATED_BODY()

   UPROPERTY()
   ETATMatchPhase Phase = ETATMatchPhase::Unstarted;
   
   // Number of seconds that the phase lasts
   // 0 implies indefinite
   UPROPERTY()
   float Duration = 0.f;

   // in server world time that the phase ends at
   // 0 implies no end
   UPROPERTY()
   float EndAt = 0.f;

   bool HasTimer() const { return Duration > 0.f && EndAt > 0.f; }
   bool operator==(const FTATReplicatedMatchPhase&) const = default;
};

UCLASS()
class TAT_API ATATGameState : public AOSEGameState
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameFrozenChanged, bool, isGameFrozen);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionOwnerPlayerStateChanged, ATATPlayerState*, missionOwnerPlayerState);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseTimerChanged, ETATMatchPhase, phase);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhaseChanged, ETATMatchPhase, phase, ETATMatchPhase, oldPhase);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerEscapedWithMatchObjective);
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnEndgameReasonAdded, ETATEndgameReason);

public:
   // static
   UFUNCTION(BlueprintPure, Category = "Game State|TAT", meta = (WorldContext = "contextObj"))
   static ATATGameState* GetTATGameState(const UObject* contextObj);
   static ATATGameState* Get(const UObject& contextObj);

   ATATGameState(const FObjectInitializer& objectInitializer);

   // from AActor
   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // AGameStateBase
   virtual void HandleBeginPlay() override;

   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   UTATMapVariationMgrComponent* GetMapVariationMgr() const { return _mapVariationMgr; }

   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   const TArray<ATATPlayerState*>& GetTATPlayerStates() const { return _tatPlayerStates; }

   // aggregate ready-check utilities
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GameState|TAT")
   void ServerResetAllPlayerReadyChecks();
   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   void GetNumPlayersReadyChecked(int& readyChecked, int& totalPlayers) const;
   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   bool GetAreAllPlayersReadyChecked() const;

   // aggregate cutscene-skip utilities
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GameState|TAT")
   void AuthorityResetAllPlayersReadyForCutsceneSkip();
   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   void GetNumPlayersReadyForCutsceneSkip(int& readyToSkipCutscene, int& totalPlayers) const;
   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   bool GetAreAllPlayersReadyForCutsceneSkip() const;

   // freeze the world for cutscenes, mission-end, etc
   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   bool IsGameFrozen() const { return _isGameFrozenCounter > 0; }
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GameState|TAT")
   void AuthoritySetGameFrozen(bool newFrozen);
   
   UPROPERTY(BlueprintAssignable, Category = "GameState|TAT")
   FOnGameFrozenChanged OnGameFrozenChanged;

   // The scheduled end time of the current phase changes (including during phase changes)
   UPROPERTY(BlueprintAssignable, Category = "GameState|TAT")
   FOnPhaseTimerChanged OnPhaseTimerChanged;

   // the current match phase has changed
   UPROPERTY(BlueprintAssignable, Category = "GameState|TAT")
   FOnPhaseChanged OnPhaseChanged;

   // which player owns our current narrative / mission state?
   // this is the host on a listen server, or, for now, the first player loaded onto a dedicated server
   UFUNCTION(BlueprintPure, Category = "GameState|TAT")
   ATATPlayerState* GetMissionOwnerPlayerState() const { return _missionOwnerPlayerState; }
   UPROPERTY(BlueprintAssignable, Category = "GameState|TAT")
   FOnMissionOwnerPlayerStateChanged OnMissionOwnerPlayerStateChanged;

   UFUNCTION(BlueprintPure, Category = "TAT|Game State")
   bool IsMissionOwner(APlayerState* playerState) const;

   UFUNCTION(BlueprintPure, Category="TAT|Game State")
   ETATMatchPhase GetCurrentPhase() const { return _matchPhase.Phase; }

   UFUNCTION(BlueprintPure, Category="TAT|Game State")
   float GetTimeLeftInPhase() const;

   UFUNCTION(BlueprintPure, Category="TAT|Game State")
   float GetCurrentPhaseDuration() const { return _matchPhase.Duration; }
   
   UFUNCTION(BlueprintPure, Category="TAT|Game State")
   float GetCurrentPhaseStart() const { return _matchPhase.EndAt - _matchPhase.Duration; }

   UFUNCTION(BlueprintPure, Category="TAT|Game State")
   float GetTotalMatchDuration() const;

   UFUNCTION(BlueprintPure, Category="TAT|Game State")
   float GetRemainingMatchDuration() const;

   void AuthorityCalculateMatchDuration();

   bool AuthorityHasEndGameReason(ETATEndgameReason reason) const;
   FOnEndgameReasonAdded OnAuthorityEndgameReasonAdded;

   //TODO: HasMatchStarted exists in the base class, so this was temp named to HasTATMatchStarted
   bool HasTATMatchStarted() const;

   void CallOrRegisterMatchStartDelegate(const FSimpleMulticastDelegate::FDelegate& startDelegate);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="TAT|Game State")
   void AuthorityStartEndgame(ETATEndgameReason reason);
   void AuthorityStartEndgameWithDuration(ETATEndgameReason reason, float duration);

   bool AuthoritySetPhaseTimeRemaining(float timeRemaining);
   void AuthorityOnPlayerEscapedWithMatchObjective();

   UPROPERTY(BlueprintAssignable)
   FOnPlayerEscapedWithMatchObjective OnPlayerEscapedWithMatchObjective;
   UFUNCTION(BlueprintPure)
   bool HasAnyPlayerEscapedWithMatchObjective() const { return _playerHasEscapedWithMatchObjective; }

   void AuthorityInitializeMapSeed(int32 mapSeed);

   UFUNCTION(BlueprintPure)
   int32 GetMapSeed() const;
   int32 GetMapSeedUnchecked() const { return _mapSeed; }

   /// Called from the game instance to notify this game state that we have new match settings data that we should replicate to clients
   void AuthorityNotifyMatchSettingsDataUpdated(TArray<uint8>&& newMatchSettingsData);

protected:
   // from AOSEGameState
   virtual void _OnOSEPlayerStateAdded(AOSEPlayerState* playerState) override;
   virtual void _OnOSEPlayerStateRemoved(AOSEPlayerState* playerState) override;

   // returns the duration in seconds of the endgame given a specific reason
   // 0 means no timer
   UFUNCTION(BlueprintNativeEvent)
   float _GetEndgameDurationSeconds(ETATEndgameReason reason) const;

   // from GameStateBase
   virtual void OnRep_ReplicatedHasBegunPlay() override;

   UPROPERTY(VisibleAnywhere, Category = "GameState|MapVariation")
   UTATMapVariationMgrComponent* _mapVariationMgr;

private:
   
   bool _AuthorityStartPhase(ETATMatchPhase phase, float duration = 0.0f);
   bool _AuthoritySetPhase(const FTATReplicatedMatchPhase& newPhase);
   void _OnPhaseTimerEnd(ETATMatchPhase phase);
   void _OnFallbackMatchTimerEnd();
   void _AuthorityOnEndgameReasonAdded(ETATEndgameReason reason);

   UFUNCTION()
   void _OnRep_PlayerHasEscapedWithMatchObjective(bool previous);
   
   bool _CanEverStartMatch() const;

   void _UpdateTATPlayerStateArray();
   void _CheckForReplicatedBeginPlay();

   UFUNCTION()
   void _AuthorityOnPlayerControllerAdded(AGameModeBase* gameMode, APlayerController* pc);
   UFUNCTION()
   void _AuthorityOnPlayerControllerRemoved(AGameModeBase* gameMode, AController* pc);

   UFUNCTION()
   void _OnRep_MatchSettings();

   UFUNCTION()
   void _OnRep_IsGameFrozen(int prevCount);
   void _BroadcastIsGameFrozenChanged();

   UFUNCTION()
   void _OnRep_MatchPhase(const FTATReplicatedMatchPhase& prevPhase);
   void _BroadcastPhaseChanged(const FTATReplicatedMatchPhase& prevPhase);

   UFUNCTION()
   void _OnRep_MissionOwner();
   void _BroadcastMissionOwnerChanged();

private:
   void _AuthorityUpdateMissionOwnerPlayerState();
   void _TryStartMatch();
   void _OnAutoExecCheats();

private:

   // Timer that ticks when the match first starts to determine when to start the game timer
   FTimerHandle _tryStartMatchTimer;

   // The game time in seconds when _tryStartMatchTimer was started
   double _tryStartMatchStartTime = -1;

   // cached version of our players cast to tat player states for quick fetching
   UPROPERTY(Transient)
   TArray<ATATPlayerState*> _tatPlayerStates;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_MissionOwner)
   ATATPlayerState* _missionOwnerPlayerState = nullptr;

   // Serialized match settings data
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_MatchSettings)
   TArray<uint8> _matchSettingsData;
   bool _hasReplicatedMatchSettings = false;

   // is the game frozen?  used by cutscenes, end-of-mission etc
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_IsGameFrozen)
   int _isGameFrozenCounter = 0;

   // Max time to wait at the beginning of a match for all players to be past the map intro
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Game State", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "seconds"))
   float _maxTimeInSecondsBeforeStartingMatchTimer = 10.0f;
   
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_MatchPhase)
   FTATReplicatedMatchPhase _matchPhase;

   UPROPERTY(Transient, Replicated)
   int32 _mapSeed;

   UPROPERTY(Transient, Replicated)
   float _matchDuration = 0.0f;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_PlayerHasEscapedWithMatchObjective)
   bool _playerHasEscapedWithMatchObjective = false;

   // a bitmask of endgame reasons that have been requested
   // authority-only for now, since those are the immediate use-cases, but it could be replicated later if desired
   uint8 _authorityEndgameReasonMask = 0;

   FSimpleMulticastDelegate _onMatchStartDelegate;
   FTimerHandle _phaseTimerHandle;
   FTimerHandle _fallbackMatchTimerHandle;
};
