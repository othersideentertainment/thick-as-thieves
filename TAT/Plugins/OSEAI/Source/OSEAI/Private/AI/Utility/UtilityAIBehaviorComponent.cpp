// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/UtilityAIBehaviorComponent.h"

// ose
#include "AI/Utility/UtilityAIBehaviorTargetInterface.h"
#include "Developer/OSEAIEditorSettings.h"

// ue4
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAIBehaviorComponent)

//---------------------------------------------------------------------------------------
// UUtilityAIBehaviorComponent
//---------------------------------------------------------------------------------------

UUtilityAIBehaviorComponent::UUtilityAIBehaviorComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
}

void UUtilityAIBehaviorComponent::BeginPlay()
{
   Super::BeginPlay();
}

void UUtilityAIBehaviorComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void UUtilityAIBehaviorComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   QUICK_SCOPE_CYCLE_COUNTER(STAT_UtilityAIBehaviorComponent_Tick);
}

void UUtilityAIBehaviorComponent::OnBehaviorTreeFinished()
{
   if (_currentState.IsValid())
   {
      if (UUtilityAIBehavior* behavior = Cast<UUtilityAIBehavior>(_currentState.Instance))
      {
         behavior->OnBehaviorTreeFinished();
      }
   }
}

#if ENABLE_VISUAL_LOG
void UUtilityAIBehaviorComponent::DescribeSelfToVisLog(FVisualLogEntry* snapshot) const
{
   Super::DescribeSelfToVisLog(snapshot);

   FVisualLogStatusCategory myCategory;
   myCategory.Category = TEXT("Utility AI Behavior");
   if (_currentState.IsValid())
   {
      myCategory.Add(TEXT("Current Behavior"), _currentState.Name.ToString());
   }
   if (_currentStateTarget.IsValid())
   {
      myCategory.Add(TEXT("Current Target"), _currentStateTarget.ToString());
   }

   for (const FStateTargetDebugLogEntry& entry : _stateTargetDebugLogEntries)
   {
      const FString scoreString = entry.WasFullyConsidered ?
         FString::Printf(TEXT("%0.3f"), entry.Score) :
         FString::Printf(TEXT("[%0.3f]"), entry.Score);

      FVisualLogStatusCategory entryCategory;
      if (entry.Target.IsValid())
      {
         const FString targetString = entry.Target.ToString();
         entryCategory.Category = FString::Printf(TEXT("%s (%s): %s"), *entry.Name.ToString(), *targetString, *scoreString);
         entryCategory.Add(TEXT("Target"), targetString);
      }
      else
      {
         entryCategory.Category = FString::Printf(TEXT("%s: %s"), *entry.Name.ToString(), *scoreString);
      }
      entryCategory.Add(TEXT("Score"), scoreString);
      entryCategory.Add(TEXT("WasFullyConsidered"), FString(entry.WasFullyConsidered ? TEXT("true") : TEXT("false")));
      entryCategory.Add(TEXT("Weight"), FString::Printf(TEXT("%0.3f"), entry.Weight));
      entryCategory.Add(TEXT("Bonus"), FString::Printf(TEXT("%0.3f"), entry.Bonus));

      for (const FConsiderationDebugLogEntry& considerationLogEntry : entry.ConsiderationLogEntries)
      {
         FVisualLogStatusCategory considerationVisualLogEntry;
         considerationVisualLogEntry.Category = considerationLogEntry.Name.ToString();
         considerationVisualLogEntry.Add(TEXT("Score"), FString::Printf(TEXT("%.02f"), considerationLogEntry.Score));
         entryCategory.AddChild(considerationVisualLogEntry);
      }

      myCategory.AddChild(entryCategory);
   }

   snapshot->Status.Add(myCategory);
}
#endif // ENABLE_VISUAL_LOG

void UUtilityAIBehaviorComponent::_OnStateEntered(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state)
{
   AOSECharacterBase* ownerCharacter = _GetOwnerCharacter();
   AActor* targetActor = target.Actor.Get();
   UUtilityAIBehavior* behavior = CastChecked<UUtilityAIBehavior>(state.Instance);
   if (ownerCharacter)
   {
      if (targetActor && targetActor->GetClass()->ImplementsInterface(UUtilityAIBehaviorTargetInterface::StaticClass()))
      {
         // let the target know that they are now the target of an AI behavior
         IUtilityAIBehaviorTargetInterface::Execute_AuthorityOnEnterTargetedByBehavior(targetActor, ownerCharacter, behavior);
      }
      _TargetActor = targetActor;
   }
}

void UUtilityAIBehaviorComponent::_OnStateExited(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state)
{
   AActor* targetActor = target.Actor.Get();
   AOSECharacterBase* ownerCharacter = _GetOwnerCharacter();
   UUtilityAIBehavior* behavior = CastChecked<UUtilityAIBehavior>(state.Instance);
   if (targetActor && ownerCharacter && targetActor->GetClass()->ImplementsInterface(UUtilityAIBehaviorTargetInterface::StaticClass()))
   {
      // let the target know that they are no longer the target of an AI behavior
      IUtilityAIBehaviorTargetInterface::Execute_AuthorityOnExitTargetedByBehavior(targetActor, ownerCharacter, behavior);
   }
}

