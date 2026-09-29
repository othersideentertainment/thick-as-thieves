// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATSavedFtueState.h"
#include "TATTutorialAction.h"
#include "Variation/Clues/TATReadableClue.h"

#include "TATCommonTutorialActions.generated.h"


class UCommonActivatableWidget;
class ATATCharacterSpawner;
class ATATTutorialRespawnPoint;
class UTATHUDHighlightWidget;
class UTATScreenWidget;
class UGameplayEffect;
enum class ETATContractState : uint8;
enum class ETATCharacter : uint8;

UENUM()
enum class ETATTutorialUISort : uint8
{
   Normal,
   AboveCommonUI
};

UCLASS(Abstract)
class TAT_API UTATTutorialAction_Instant : public UTATTutorialAction
{
   GENERATED_BODY()

public:
   virtual void RunInstant(const FTATTutorialParams& params) const {}

   // Must always eventually call next
   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override
   {
      RunInstant(params);
      next();
      return {};
   }
};

UCLASS(Abstract, Blueprintable, Meta = (ShowWorldContextPin))
class TAT_API UTATTutorialAction_InstantBlueprintBase : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

public:
   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UFUNCTION(BlueprintImplementableEvent)
   void BP_Run(const FTATTutorialParams& params) const;

   UFUNCTION(BlueprintNativeEvent)
   FString BP_GetDebugName() const;

   UFUNCTION(BlueprintImplementableEvent)
   void BP_Validate(const FTATTutorialValidationParams& params) const;
};

UCLASS(Abstract, Blueprintable, Meta = (ShowWorldContextPin))
class TAT_API UTATTutorialAction_CleanupBlueprintBase : public UTATTutorialAction
{
   GENERATED_BODY()

public:
   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UFUNCTION(BlueprintImplementableEvent)
   bool BP_Run(const FTATTutorialParams& params, UObject*& outCleanupParam) const;

   UFUNCTION(BlueprintImplementableEvent)
   void BP_Cleanup(UObject* cleanupParam) const;

   UFUNCTION(BlueprintNativeEvent)
   FString BP_GetDebugName() const;

   UFUNCTION(BlueprintImplementableEvent)
   void BP_Validate(const FTATTutorialValidationParams& params) const;
};

// Spawns an actor at the location of the specified actor
UCLASS(DisplayName="Spawn Actor")
class TAT_API UTATTutorialAction_SpawnActor : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif
   
   UPROPERTY(EditAnywhere)
   TSubclassOf<AActor> ActorClassToSpawn;

   // The actor in the level to use as the location to spawn at
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<AActor> ActorToSpawnAt;
};

// Spawns a temporary actor at the location of the specified actor,
// and destroys it at the end of the step
UCLASS(DisplayName="Spawn Actor During Step")
class TAT_API UTATTutorialAction_SpawnActorDuringStep : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere)
   TSubclassOf<AActor> ActorClassToSpawn;

   // The actor in the level to use as the location to spawn at
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<AActor> ActorToSpawnAt;
};

// Waits the specified number of seconds
UCLASS(DisplayName="Wait")
class TAT_API UTATTutorialAction_Wait : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;

   UPROPERTY(EditAnywhere, meta=(UIMin="0.1", ClampMin="0.1"))
   float DelaySeconds = 0;
};

// Triggers a design analytics event if the local player triggers a gameplay event with a matching tag
UCLASS(DisplayName="Progression Analytics Event")
class TAT_API UTATTutorialAction_ProgressionAnalyticsEvent: public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;

protected:
   UPROPERTY(EditAnywhere)
   FString _AnalyticsSectionName;
   
   UPROPERTY(EditAnywhere)
   FString _AnalyticsEventName;
};

// Triggers a design analytics event if the local player triggers a gameplay event with a matching tag
UCLASS(DisplayName="Conditional Design Analytics Event [Gameplay Event]")
class TAT_API UTATTutorialAction_ConditionalDesignAnalyticsEvent_GameplayEvent : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;

protected:
   UPROPERTY(EditAnywhere)
   FGameplayTag _EventTagToMatch;
   UPROPERTY(EditAnywhere)
   FString _AnalyticsEventName;
};

// Triggers a design analytics event if the local player triggers a gameplay event with a matching tag
UCLASS(DisplayName="Conditional Design Analytics Event [Gameplay Tag Added/Removed]")
class TAT_API UTATTutorialAction_ConditionalDesignAnalyticsEvent_GameplayTag : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;

protected:
   UPROPERTY(EditAnywhere)
   FGameplayTag _TagToMatch;
   UPROPERTY(EditAnywhere)
   bool _TriggerOnAdd;
   UPROPERTY(EditAnywhere)
   FString _AnalyticsEventName;
};

// Shows a UI Overlay
//
// Not sure how much this will ultimately work as a BP hybrid, so may replace
UCLASS(Abstract, Blueprintable, Meta = (ShowWorldContextPin))
class TAT_API UTATTutorialAction_AddOverlayUIBase : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;

public:
   UPROPERTY(EditAnywhere)
   ETATTutorialUISort SortOrder = ETATTutorialUISort::Normal;
   
   UFUNCTION(BlueprintNativeEvent)
   FString _GetDebugName() const;
   
   UFUNCTION(BlueprintNativeEvent)
   UUserWidget* CreateWidget(APlayerController* controller) const;
};

// Just an example of a modal popup
UCLASS(Abstract, Blueprintable, Meta = (ShowWorldContextPin))
class TAT_API UTATTutorialAction_ShowScreenBase : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;

public:
   UPROPERTY(EditAnywhere)
   ETATTutorialUISort SortOrder = ETATTutorialUISort::Normal;
   
   UFUNCTION(BlueprintNativeEvent)
   FString _GetDebugName() const;
   
   UFUNCTION(BlueprintNativeEvent)
   UTATScreenWidget* CreateScreenWidget(APlayerController* controller) const;
   
   UPROPERTY(EditAnywhere)
   bool _ShouldTriggerAnalyticsEvent { false };
   
   UPROPERTY(EditAnywhere, meta=(EditCondition="_ShouldTriggerAnalyticsEvent"))
   FString _AnalyticsSectionName;
   
   UPROPERTY(EditAnywhere, meta=(EditCondition="_ShouldTriggerAnalyticsEvent"))
   FString _AnalyticsEventName;
};

// Swallows back input for the screen for the duration of this step
UCLASS(DisplayName="Swallow Back Action (CommonUI)")
class TAT_API UTATTutorialAction_SwallowBackAction : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   UPROPERTY(EditAnywhere)
   TSoftClassPtr<class UTATActivatableWidget> WidgetClass;
};

// Shows a highlight of a specified HUD element
UCLASS(Meta = (DisplayName="Show HUD Highlight"))
class TAT_API UTATTutorialAction_ShowHUDHighlight : public UTATTutorialAction_AddOverlayUIBase
{
   GENERATED_BODY()


public:
   virtual UUserWidget* CreateWidget_Implementation(APlayerController* controller) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   // The hud element to highlight
   UPROPERTY(EditAnywhere, meta=(Categories="HudElement"))
   FGameplayTag HudElement;
   
   UPROPERTY(EditAnywhere)
   TSubclassOf<UTATHUDHighlightWidget> HighlightWidgetClass;
};

// Applies the specified gameplay effect
UCLASS(DisplayName="Apply Gameplay Effect")
class TAT_API UTATTutorialAction_ApplyGameplayEffect : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   // the effect to apply
   UPROPERTY(EditAnywhere)
   TSubclassOf<UGameplayEffect> Effect;

   // Whether to remove the effect at the end of the step
   UPROPERTY(EditAnywhere)
   bool RemoveOnStepEnd = false;
};

// Removes the specified gameplay effect
UCLASS(DisplayName="Remove Gameplay Effect")
class TAT_API UTATTutorialAction_RemoveGameplayEffect : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   // the effect to remove
   UPROPERTY(EditAnywhere)
   TSubclassOf<UGameplayEffect> Effect;

   // Number of stacks to remove (-1 is all of them)
   UPROPERTY(EditAnywhere)
   int StacksToRemove = -1;
};


UCLASS(DisplayName="Spawn Readable Clue")
class TAT_API UTATTutorialAction_SpawnReadableClue : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   // The clue spawner to spawn the readable clue at
   UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/TAT.TATClueActorSpawner"))
   TSoftObjectPtr<AActor> ClueSpawner;

   UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
   FTATReadableClue Clue;
};

// Sets text of the objective line
UCLASS(DisplayName="Set Objective Text")
class TAT_API UTATTutorialAction_SetObjectiveText : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;
   
   UPROPERTY(EditAnywhere)
   FText ObjectiveText;
   
   UPROPERTY(EditAnywhere)
   bool RemoveOnStepEnd = false;
};


UENUM()
enum class ETATTutorialActorVisibility : uint8
{
   Visible,
   Hidden,
};

// Shows or hides an actor via HiddenInGame
// TODO: add optional interface to customize behavior
UCLASS(DisplayName="Show/Hide Actor")
class TAT_API UTATTutorialAction_SetActorHidden : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   // The actor in the level to show
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<AActor> Actor;
   
   UPROPERTY(EditAnywhere)
   ETATTutorialActorVisibility Visibility;
   
   UPROPERTY(EditAnywhere)
   bool RevertOnStepEnd = false;
};


// Set the current respawn point
UCLASS(DisplayName="Set Respawn Point")
class TAT_API UTATTutorialAction_SetRespawnPoint : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   // The actor in the level to show
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<ATATTutorialRespawnPoint> RespawnPoint;
};

// Destroys an NPC associated with a checkpoint, and prevents it from being respawned
UCLASS(DisplayName="Destroy NPC")
class TAT_API UTATTutorialAction_DestroyNPC : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   // The spawner for the guard to destroy
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<ATATCharacterSpawner> GuardSpawner;
};

UCLASS(DisplayName="Slomo")
class TAT_API UTATTutorialAction_Slomo : public UTATTutorialAction
{
   GENERATED_BODY()

   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const override;
   virtual FString GetDebugName() const override;

   UPROPERTY(EditAnywhere)
   float TimeDilation = 1;
};

UCLASS(DisplayName="Save FTUE Checkpoint")
class TAT_API UTATTutorialAction_SetSavedFtueState : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

public:
   
   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
   
   UPROPERTY(EditAnywhere)
   ETATSavedFtueState NewState = ETATSavedFtueState::InThievesDen;

   // Doesn't set the checkpoint if the FTUE is already past this state
   UPROPERTY(EditAnywhere)
   bool OnlyIfLower = true;
};

UCLASS(DisplayName="Set Contract State")
class TAT_API UTATTutorialAction_SetContractState : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

public:
   
   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
   
   UPROPERTY(EditAnywhere, meta=(Categories="Contract"))
   FGameplayTag Contract;

   UPROPERTY(EditAnywhere)
   ETATContractState NewState;
};

UCLASS(DisplayName="Unlock Content")
class TAT_API UTATTutorialAction_UnlockContent : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

public:
   
   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
   
   UPROPERTY(EditAnywhere, meta=(Categories="UnlockableCategory"))
   FGameplayTag UnlockableTag;
};

UCLASS(DisplayName="Set Saved Character")
class TAT_API UTATTutorialAction_SetSavedCharacter : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

public:
   
   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
   
   UPROPERTY(EditAnywhere)
   ETATCharacter Character;
};

UCLASS(DisplayName = "GA FTUE started")
class TAT_API UTATTutorialAction_GAFTUEStarted : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

public:

   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
};

UCLASS(DisplayName = "GA FTUE completed")
class TAT_API UTATTutorialAction_GAFTUECompleted : public UTATTutorialAction_Instant
{
   GENERATED_BODY()

public:

   virtual void RunInstant(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
};

