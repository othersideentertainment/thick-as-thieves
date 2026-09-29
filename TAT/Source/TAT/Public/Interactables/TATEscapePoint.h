// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"
#include "OSECoreCheats.h" // To determine if cheats are enabled or not

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATEscapePoint.generated.h"

class ATATGenericIndicator;
class UGameplayEffect;
class UTATMapActorComponent;
class UTATSpawnerComponent;
class UTATOverlapTargetTriggerComponent;
enum class ETATOverlapTargetTriggerReason : uint8;

UENUM(BlueprintType)
enum class ETATEscapePointState : uint8
{
   Dormant, // Not scheduled to open
   Pending, // Scheduled to open
   Summonable,
   Open, // Open
   Closed // Previously open, but now closed
};

USTRUCT()
struct TAT_API FTATEscapePointOpenState
{
   GENERATED_BODY()
public:
   UPROPERTY()
   ETATEscapePointState State = ETATEscapePointState::Dormant;

   /// Time on the server we transitioned to an open state
   UPROPERTY()
   float ServerTimeOpened = 0.0f;
};

UENUM(BlueprintType)
enum class ETATEscapeSummonState : uint8
{
   None,
   Summoning,
   PartialTeam,
   Error,
};

USTRUCT()
struct TAT_API FTATEscapePointSummonState
{
   GENERATED_BODY()
public:
   UPROPERTY()
   ETATEscapeSummonState State = ETATEscapeSummonState::None;

   UPROPERTY()
   float StartedAt = 0.0f;
};

UCLASS(Blueprintable)
class TAT_API ATATEscapePoint : public AActor

{
   GENERATED_BODY()

public:
   ATATEscapePoint();

   
#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   UFUNCTION(BlueprintImplementableEvent)
   void UpdateEscapeRouteVisuals(bool bCanBeUsed, ETATEscapePointState state);

   // only called when the state actually changes
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnStateChanged(ETATEscapePointState state, ETATEscapePointState previousState);

   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnSummonStateChanged(ETATEscapeSummonState state, ETATEscapeSummonState previousState, bool wasRecent);

   // Called on host + all clients (not dedicated server) when an escape interaction starts / stops
   UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
   void UpdateEscapeInProgressVisuals(bool escapeInProgress);

   /// Player-facing name for an escape route
   UPROPERTY(BlueprintReadOnly, EditAnywhere)
   FText EscapePointName;

   /// Identifier for an escape route instance. Used for gating rewards behind a specific escape route
   UPROPERTY(EditInstanceOnly, Category = "TAT", Meta = (Categories = "EscapeRoute"))
   FGameplayTag Identifier;

   UFUNCTION(BlueprintCallable)
   bool IsEscapeUsableNow() const;

   UFUNCTION(BlueprintPure)
   ETATEscapePointState GetState() const { return _openState.State; }

   UFUNCTION(BlueprintPure)
   float GetOpenTimeRemaining() const;

   UFUNCTION(BlueprintPure)
   float GetSummonTimeRemaining() const;

#if OSE_CHEATS_ENABLED
   void CheatOpenEscape() { _AuthorityOpenEscape(); }
#endif



protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type endPlayReason) override;
   virtual void PostInitializeComponents() override;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_OpenState)
   FTATEscapePointOpenState _openState;

   UFUNCTION()
   void _OnRep_OpenState(FTATEscapePointOpenState oldOpenState);

   UFUNCTION()
   void _OnRep_SummonState(const FTATEscapePointSummonState& summonState);

   UFUNCTION()
   void _AuthorityOnEscapeSpawnerSpawned(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   UFUNCTION()
   void _AuthorityOnMatchTimerUpdated(ETATMatchPhase phase);

   void _AuthorityOnMatchStart();
   void _AuthorityOnEscapeBegin();

   void _AuthorityOpenEscape();

   void _AuthorityOnEscapeEnd();

   void _AuthoritySetState(ETATEscapePointState state);
   void _UpdateEscapeForState();

   void _UpdateMapSprite();
   void _OnStateChanged(ETATEscapePointState previousState);

   void _AuthorityOnSummonTargetFound(ETATOverlapTargetTriggerReason reason);
   void _AuthorityOnSummonTargetLost(AActor* lostTarget);
   void _AuthorityUpdateSummonState();
   void _AuthorityOnSummonComplete();

   void _ApplySummonEffectsToTargets();
   void _AuthorityAddTeamChangeListeners();
   UFUNCTION()
   void _AuthorityHandleTargetInTeamChange(bool inTeam);

   // Time in seconds from endgame start that the escape route opens
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TAT|Time", meta = (DisplayName = "Escape Open Start Time", Units = "seconds", UIMin = 0, ClampMin = 0))
   float _escapeOpenStartTimeFromEndgame = 0;

   // Duration that the escape is open in seconds
   // 0 = indefinite
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TAT|Time", meta = (Units = "seconds", UIMin = 0, ClampMin = 0, EditCondition))
   float _escapeOpenDuration = 180.f;
   
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TAT|Time", meta = (InlineEditConditionToggle))
   bool _escapeClosesOnTimer = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Summon", meta = (Units = "seconds", UIMin = 0, ClampMin = 0, EditCondition))
   float _summonDuration = 10.f;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Summon", Meta = (AllowAbstract = true))
   TSubclassOf<AActor> _requiredFinalActorClass;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TAT|Interactable")
   bool _canBeUsedMultipleTimes { false };

   // A spawner component that will randomly select a subset of escape points to be usable at match end
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TAT|Spawner")
   UTATSpawnerComponent* _usableInEscapeSpawner = nullptr;



private:

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_SummonState)
   FTATEscapePointSummonState _summonState;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Map")
   UTATMapActorComponent* _mapActorComponent = nullptr;

   // Map sprite entry used when an escape route can be used
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Map", Meta = (Categories = "MapSprite"))
   FGameplayTag _escapeRouteOpenMapSpriteEntry;

   // Map sprite entry used when an escape route has been taken by another player, and can no longer be used
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Map", Meta = (Categories = "MapSprite"))
   FGameplayTag _escapeRouteTakenMapSpriteEntry;

   // Map sprite entry used when an escape route has not opened yet
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Map", Meta = (Categories = "MapSprite"))
   FGameplayTag _escapeRoutePendingMapSpriteEntry;

   // Map sprite entry used when an escape route has not opened yet
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Map", Meta = (Categories = "MapSprite"))
   FGameplayTag _escapeRouteSummonableMapSpriteEntry;

   // Map sprite entry used for dormant escape routes (this will likely be the default state)
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Map", Meta = (Categories = "MapSprite"))
   FGameplayTag _escapeRouteDormantMapSpriteEntry;

   // Gameplay effect when in summoning
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Summon", Meta = (Categories = "MapSprite"))
   TSubclassOf<UGameplayEffect> _summoningEffect;

   // Gameplay effect when not all allies are in the zone
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Summon", Meta = (Categories = "MapSprite"))
   TSubclassOf<UGameplayEffect> _partialTeamEffect;

   // Gameplay effect when in summoning
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Summon", Meta = (Categories = "MapSprite"))
   TSubclassOf<UGameplayEffect> _errorEffect;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Compass")
   TSubclassOf<ATATGenericIndicator> _compassIndicatorClass;

   UPROPERTY(Transient)
   TObjectPtr<ATATGenericIndicator> _compassIndicator;

   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _actorsWithEffectApplied;

   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATOverlapTargetTriggerComponent> _summonOverlapTrigger;

   FTimerHandle _nextStateTimer;
};
