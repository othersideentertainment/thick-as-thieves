// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"

// ose
#include "Abilities/OSEAbilityTask.h"
#include "Abilities/OSEHeldActionCues.h"

#include "AbilityTask_PlayHeldActionCues.generated.h"

class UAbilitySystemComponent;

UENUM()
enum class EOSEHeldActionAbilityTaskState : uint8
{
   None,
   HoldInProgress,
   HoldCompleted,
   HoldInterrupted
};

/// -------------------------------------------------------------------------------------------------------------------------
// UAbilityTask_PlayHeldActionCues:
// Designed for playing SFX (via GameplayCues) tied to a press-and-hold behavior of a GameplayAbility. 
// 
// After calling PlayHeldActionSFX, users should call OnHeldActionCompleted() if the associated press-and-hold action runs to 
// completion, or OnHeldActionInterrupted() if it is interrupted. 
// 
// If the ability ends before either is called, OnHeldActionInterrupted() will be called automatically when the task ends.
/// -------------------------------------------------------------------------------------------------------------------------
UCLASS()
class OSECORE_API UAbilityTask_PlayHeldActionCues : public UOSEAbilityTask
{
   GENERATED_BODY()

public:
   UAbilityTask_PlayHeldActionCues(const FObjectInitializer& objectInitializer);

   // Spawns task to manage addition/execution of specified gameplay cues. 
   // Accepts a struct of cues to be played for each press-and-hold event (held | completed | released-early) and an optional duration to be passed along in the (held) cue params.
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
   static UAbilityTask_PlayHeldActionCues* PlayHeldActionCues(UGameplayAbility* owningAbility, const FOSEHeldActionCues& heldActionCues, const float holdDuration);

protected:
   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

   virtual void OnDestroy(bool abilityIsEnding) override;

   // Called when press-and-hold action is successfully completed without interruption.
   UFUNCTION(BlueprintCallable)
   void OnHeldActionCompleted();

   // Called when press-and-hold action is interrupted before duration is elapsed, 
   UFUNCTION(BlueprintCallable)
   void OnHeldActionInterrupted();

private:

   void _HandleCompleteAction();

   void _HandleInterruptAction();

   // Executes a gameplay cue on the target.
   void _TryExecuteGameplayCue(const FGameplayTag& gameplayCueTag);

   // Adds a persistent gameplay cue to the target.
   void _TryAddGameplayCue(const FGameplayTag& gameplayCueTag);

   // Removes a persistent gameplay cue from the target.
   void _TryRemoveGameplayCue(const FGameplayTag& gameplayCueTag);


private:
   // Holds tags for gameplay cues to be activated
   FOSEHeldActionCues _heldActionCues;

   // Duration to be passed along in _TryAddGameplayCue()
   float _holdDuration = 0.f;

   // Keeps track of current state in press-and-hold sequence
   EOSEHeldActionAbilityTaskState _holdState = EOSEHeldActionAbilityTaskState::None;
};
