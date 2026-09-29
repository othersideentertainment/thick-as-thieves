// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Tasks/TATAsyncTaskAIStatesChanged.h"

// tat
#include "AI/TATAIController.h"
#include "AI/Escalation/TATEscalationComponent.h"

// ose
#include "AI/Alertness/OSEAlertnessInterface.h"

// ue
#include "AIController.h"
#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAsyncTaskAIStatesChanged)

DEFINE_LOG_CATEGORY(LogAsyncTaskAIStatesChanged);

UTATAsyncTaskAIStatesChanged* UTATAsyncTaskAIStatesChanged::ListenForStateChanges(UObject* worldContextObject, ATATAIController* aiController)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);

   if (!IsValid(aiController))
   {
      UE_LOG(LogAsyncTaskAIStatesChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAIStatesChanged, null AI Controller!"), *worldContextObject->GetName());
      return nullptr;
   }

   APawn* aiPawn = aiController->GetPawn();
   if (!IsValid(aiPawn))
   {
      UE_LOG(LogAsyncTaskAIStatesChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAIStatesChanged, AI Controller has not yet possessed a pawn!"), *worldContextObject->GetName());
      return nullptr;
   }

   IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(aiPawn);
   if (alertnessInterface == nullptr)
   {
      UE_LOG(LogAsyncTaskAIStatesChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAIStatesChanged, could not find the alertness component!"), *worldContextObject->GetName());
      return nullptr;
   }

   UOSEAlertnessComponent* alertnessComponent = alertnessInterface->GetAlertnessComponent();
   if (!IsValid(alertnessComponent))
   {
      UE_LOG(LogAsyncTaskAIStatesChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAIStatesChanged, could not find the alertness component!"), *worldContextObject->GetName());
      return nullptr;
   }

   UTATEscalationComponent* escalationComponent = aiController->GetTATEscalationComponent();
   if (!IsValid(escalationComponent))
   {
      UE_LOG(LogAsyncTaskAIStatesChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAIStatesChanged, could not find the escalation component!"), *worldContextObject->GetName());
      return nullptr;
   }

   UTATAsyncTaskAIStatesChanged* waitForAIStateChangeTask = NewObject<UTATAsyncTaskAIStatesChanged>();

   // Alertness binding
   waitForAIStateChangeTask->_alertnessComponent = alertnessComponent;
   waitForAIStateChangeTask->_alertnessComponent->OnAlertnessLevelChanged.AddUniqueDynamic(
      waitForAIStateChangeTask, &UTATAsyncTaskAIStatesChanged::_OnAlertnessLevelChanged);

   // Alertness initial value caching (event dispatcher won't be picked up until this method completes)
   EAlertnessLevel level = alertnessComponent->GetAlertnessLevel();
   waitForAIStateChangeTask->_OnAlertnessLevelChanged(level, level);

   // Escalation binding
   waitForAIStateChangeTask->_escalationComponent = escalationComponent;
   waitForAIStateChangeTask->_escalationComponent->OnEscalationStateChanged.AddUniqueDynamic(
      waitForAIStateChangeTask, &UTATAsyncTaskAIStatesChanged::_OnEscalationStateChanged);

   // Escalation initial value caching (event dispatcher won't be picked up until this method completes)
   waitForAIStateChangeTask->_OnEscalationStateChanged(escalationComponent->GetCurrentState());

   return waitForAIStateChangeTask;
}

void UTATAsyncTaskAIStatesChanged::Activate()
{
   Super::Activate();

   if (TriggerEventsOnActivation)
   {
      const FTATAIStateCombination dummy;
      OnAIAlertnessChanged.Broadcast(dummy, _currentStateCombo);
      OnAIEscalationChanged.Broadcast(dummy, _currentStateCombo);
   }
}

void UTATAsyncTaskAIStatesChanged::EndTask()
{
   if (IsValid(_alertnessComponent))
   {
      _alertnessComponent->OnAlertnessLevelChanged.RemoveAll(this);
   }

   if (IsValid(_escalationComponent))
   {
      _escalationComponent->OnEscalationStateChanged.RemoveAll(this);
   }

   SetReadyToDestroy();
   MarkAsGarbage();
}

void UTATAsyncTaskAIStatesChanged::_OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel)
{
   const FTATAIStateCombination previous = _currentStateCombo;
   _currentStateCombo.Alertness = newAlertnessLevel;
   _currentStateCombo.UseAlertnessFilter = true;
   OnAIAlertnessChanged.Broadcast(previous, _currentStateCombo);
}

void UTATAsyncTaskAIStatesChanged::_OnEscalationStateChanged(const ETATEscalationState newEscalationState)
{
   const FTATAIStateCombination previous = _currentStateCombo;
   _currentStateCombo.Escalation = newEscalationState;
   _currentStateCombo.UseEscalationFilter = true;
   OnAIEscalationChanged.Broadcast(previous, _currentStateCombo);
}
