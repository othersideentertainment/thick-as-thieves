// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_PlayHeldActionCues.h"

// ue4
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PlayHeldActionCues)

DEFINE_LOG_CATEGORY_STATIC(LogOSEAbilityTask_PlayHeldActionCues, Log, All);

UAbilityTask_PlayHeldActionCues::UAbilityTask_PlayHeldActionCues(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   bTickingTask = false;
}

void UAbilityTask_PlayHeldActionCues::Activate()
{
   Super::Activate();

   _holdState = EOSEHeldActionAbilityTaskState::HoldInProgress;

   UE_LOG(LogOSEAbilityTask_PlayHeldActionCues, Verbose, TEXT("Begin press-and-hold action (adding cue: %s)...")
      , *_heldActionCues.Cue_HoldDuration.ToString());

   // Add "press-and-hold duration" cue
   _TryAddGameplayCue(_heldActionCues.Cue_HoldDuration);
}

void UAbilityTask_PlayHeldActionCues::OnDestroy(bool abilityIsEnding)
{
   // If ability ended before press-and-hold completed, handle the interruption
   if(_holdState == EOSEHeldActionAbilityTaskState::HoldInProgress)
   {
      _HandleInterruptAction();
   }

   Super::OnDestroy(abilityIsEnding);
}

// static
UAbilityTask_PlayHeldActionCues* UAbilityTask_PlayHeldActionCues::PlayHeldActionCues(UGameplayAbility* owningAbility, const FOSEHeldActionCues& heldActionCues, const float holdDuration)
{
   UAbilityTask_PlayHeldActionCues* abilityTask = NewAbilityTask<UAbilityTask_PlayHeldActionCues>(owningAbility);
   abilityTask->_heldActionCues = heldActionCues;
   abilityTask->_holdDuration = holdDuration;

   return abilityTask;
}

void UAbilityTask_PlayHeldActionCues::OnHeldActionCompleted()
{
   // Make sure we're transitioning from a hold-in-progress
   if (_holdState != EOSEHeldActionAbilityTaskState::HoldInProgress)
   {
      UE_LOG(LogOSEAbilityTask_PlayHeldActionCues, Warning, TEXT("OnHeldActionCompleted() called at unexpected state (%s)")
         , *UEnum::GetValueAsString(_holdState));
      return;
   }
   
   _HandleCompleteAction();
   EndTask();
}

void UAbilityTask_PlayHeldActionCues::OnHeldActionInterrupted()
{
   // Make sure we're transitioning from a hold-in-progress
   if (_holdState != EOSEHeldActionAbilityTaskState::HoldInProgress)
   {
      UE_LOG(LogOSEAbilityTask_PlayHeldActionCues, Warning, TEXT("OnHeldActionInterrupted() called at unexpected state (%s)")
         , *UEnum::GetValueAsString(_holdState));
      return;
   }
   
   _HandleInterruptAction();
   EndTask();
}

void UAbilityTask_PlayHeldActionCues::_HandleCompleteAction()
{
   // Make sure we're transitioning from a hold-in-progress
   check(_holdState == EOSEHeldActionAbilityTaskState::HoldInProgress);
   _holdState = EOSEHeldActionAbilityTaskState::HoldCompleted;

   UE_LOG(LogOSEAbilityTask_PlayHeldActionCues, Verbose, TEXT("Press-and-hold completed! (executing cue: %s)")
      , *_heldActionCues.Cue_HoldCompleted.ToString());

   // Remove "press-and-hold duration" cue
   _TryRemoveGameplayCue(_heldActionCues.Cue_HoldDuration);

   // Execute "press-and-hold completed" cue
   _TryExecuteGameplayCue(_heldActionCues.Cue_HoldCompleted);
}

void UAbilityTask_PlayHeldActionCues::_HandleInterruptAction()
{
   // Make sure we're transitioning from a hold-in-progress
   check(_holdState == EOSEHeldActionAbilityTaskState::HoldInProgress);
   _holdState = EOSEHeldActionAbilityTaskState::HoldInterrupted;

   UE_LOG(LogOSEAbilityTask_PlayHeldActionCues, Verbose, TEXT("Press-and-hold interrupted! (executing cue: %s)")
      , *_heldActionCues.Cue_HoldInterrupted.ToString());

   // Remove "press-and-hold duration" cue
   _TryRemoveGameplayCue(_heldActionCues.Cue_HoldDuration);

   // Execute "press-and-hold interrupted" cue
   _TryExecuteGameplayCue(_heldActionCues.Cue_HoldInterrupted);
}

void UAbilityTask_PlayHeldActionCues::_TryExecuteGameplayCue(const FGameplayTag& gameplayCueTag)
{
   // Make sure we have a valid target!
   check(AbilitySystemComponent != nullptr);

   if (gameplayCueTag.IsValid())
   {
      AbilitySystemComponent->ExecuteGameplayCue(gameplayCueTag);
   }
}

void UAbilityTask_PlayHeldActionCues::_TryAddGameplayCue(const FGameplayTag& gameplayCueTag)
{
   // Make sure we have a valid target!
   check(AbilitySystemComponent != nullptr);

   if (gameplayCueTag.IsValid())
   {
      // Pass duration through gameplay cue params
      FGameplayCueParameters gameplayCueParams;
      gameplayCueParams.RawMagnitude = _holdDuration;

      AbilitySystemComponent->AddGameplayCue(gameplayCueTag, gameplayCueParams);
   }
}

void UAbilityTask_PlayHeldActionCues::_TryRemoveGameplayCue(const FGameplayTag& gameplayCueTag)
{
   // Make sure we have a valid target!
   check(AbilitySystemComponent != nullptr);

   if (gameplayCueTag.IsValid())
   {
      AbilitySystemComponent->RemoveGameplayCue(gameplayCueTag);
   }
}

