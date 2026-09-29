// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/UtilityAIGoalComponent.h"

// ose
#include "AI/OSEAIController.h"
#include "AI/BehaviorTree/Blackboard/BlackboardKeyType_UtilityStateTarget.h"
#include "AI/Utility/UtilityAITypes.h"
#include "AI/Utility/UtilityAIBehaviorTargetInterface.h"

// ue
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Misc/DataValidation.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAIGoalComponent)


//---------------------------------------------------------------------------------------
// UUtilityAIGoalComponent
//---------------------------------------------------------------------------------------

UUtilityAIGoalComponent::UUtilityAIGoalComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   _shouldRegisterAsGoalUtilityBehavior = true;
}

void UUtilityAIGoalComponent::BeginPlay()
{
   Super::BeginPlay();

   // installing our own states on this component!
   SetStatesOwner(this);

   // Validate and instantiate all our states from the goal sets.
   _states.Reset();
   for (const UUtilityGoalSet* goalSet : _goalSets)
   {
      if (!goalSet)
      {
         UE_LOG(LogOSEUtilityAI, Warning, TEXT("Ignoring empty UtilityGoalSet in %s."), *GetPathName());
         continue;
      }

      for (const FUtilityGoalSetItem& setItem : goalSet->Goals)
      {
         if (setItem.Enabled)
            FUtilityStateEvaluatorInstance::CreateAndAdd(*this, *this, setItem.Name, setItem.Weight, setItem.MomentumBonus, setItem.Evaluator, setItem.Goal, _states);
      }
   }

   // Stable sort behaviors by weight highest to lowest.
   _states.StableSort([](const FUtilityStateEvaluatorInstance& a, const FUtilityStateEvaluatorInstance& b)
   {
      return a.Weight > b.Weight;
   });
}

void UUtilityAIGoalComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void UUtilityAIGoalComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   QUICK_SCOPE_CYCLE_COUNTER(STAT_UtilityAIGoalComponent_Tick);
}

#if ENABLE_VISUAL_LOG
void UUtilityAIGoalComponent::DescribeSelfToVisLog(FVisualLogEntry* snapshot) const
{
   Super::DescribeSelfToVisLog(snapshot);

   FVisualLogStatusCategory myCategory;
   myCategory.Category = TEXT("Utility AI Goal");
   if (_currentState.IsValid())
   {
      myCategory.Add(TEXT("Current Goal"), _currentState.Name.ToString());
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

      for(const FConsiderationDebugLogEntry& considerationLogEntry : entry.ConsiderationLogEntries)
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

void UUtilityAIGoalComponent::_OnStateEntered(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state)
{
   AOSECharacterBase* ownerCharacter = _GetOwnerCharacter();
   AActor* targetActor = target.Actor.Get();
   if (ownerCharacter)
   {
      if (targetActor && targetActor->GetClass()->ImplementsInterface(UUtilityAIBehaviorTargetInterface::StaticClass()))
      {
         // let the target know that they are now the target of an AI behavior
         IUtilityAIBehaviorTargetInterface::Execute_AuthorityOnEnterTargetedByGoal(targetActor, ownerCharacter, state.Instance);
      }
   }
}

void UUtilityAIGoalComponent::_OnStateExited(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state)
{
   AActor* targetActor = target.Actor.Get();
   AOSECharacterBase* ownerCharacter = _GetOwnerCharacter();
   if (targetActor && ownerCharacter && targetActor->GetClass()->ImplementsInterface(UUtilityAIBehaviorTargetInterface::StaticClass()))
   {
      // let the target know that they are no longer the target of an AI behavior
      IUtilityAIBehaviorTargetInterface::Execute_AuthorityOnExitTargetedByGoal(targetActor, ownerCharacter, state.Instance);
   }
}

void UUtilityAIGoalComponent::_OnStateChanged(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state)
{
   UBlackboardComponent* blackboardComp = IsValid(_aiController) ? _aiController->GetBlackboardComponent() : nullptr;
   if (blackboardComp)
   {
      // target key
      if (!_blackboardKeyTargetActorName.IsNone())
      {
         if (target.IsValid())
         {
            blackboardComp->SetValue<UBlackboardKeyType_Object>(_blackboardKeyTargetActorName, target.GetTargetUObject());
         }
         else
         {
            blackboardComp->SetValue<UBlackboardKeyType_Object>(_blackboardKeyTargetActorName, nullptr);
         }
      }

      // goal class key
      if (!_blackboardKeyGoalObjectName.IsNone())
      {
         if (state.IsValid())
         {
            blackboardComp->SetValue<UBlackboardKeyType_Object>(_blackboardKeyGoalObjectName, state.Instance);
         }
         else
         {
            blackboardComp->SetValue<UBlackboardKeyType_Object>(_blackboardKeyGoalObjectName, nullptr);
         }
      }

      // utility state target key
      if (!_blackboardKeyUtilityStateTarget.IsNone())
      {
         if (state.IsValid())
         {
            blackboardComp->SetValue<UBlackboardKeyType_UtilityStateTarget>(_blackboardKeyUtilityStateTarget, target);
         }
         else
         {
            blackboardComp->SetValue<UBlackboardKeyType_UtilityStateTarget>(_blackboardKeyUtilityStateTarget, UBlackboardKeyType_UtilityStateTarget::InvalidValue);
         }
      }
   }
}

#if WITH_EDITOR
void UUtilityAIGoalComponent::_RefreshWeights()
{
   WeightInfo.Reset();

   TArray<UtilityAIWeightHelper::WeightInfo> weightInfos;

   for (UUtilityGoalSet* goalSet : _goalSets)
   {
      if (!goalSet)
         continue;

      for (FUtilityGoalSetItem& goalSetItem : goalSet->Goals)
      {
         if (goalSetItem.Evaluator && goalSetItem.Enabled)
         {
            weightInfos.Add({ goalSetItem.Name, goalSetItem.Weight });
         }
      }
   }

   weightInfos.Sort();

   for (const UtilityAIWeightHelper::WeightInfo& info : weightInfos)
   {
      WeightInfo.Add(FString::Printf(TEXT("%.02f: %s"), info.Weight, *info.Name.ToString()));
   }
}

EDataValidationResult UUtilityAIGoalComponent::IsDataValid(FDataValidationContext& context)
{
   for (int idx = 0; idx < _goalSets.Num(); ++idx)
   {
      UUtilityGoalSet* goalSet = _goalSets[idx];
      if (!goalSet)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no goal set at index %d!"), *GetName(), idx)));
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UUtilityAIGoalComponent::PostLoad()
{
   Super::PostLoad();
   _RefreshWeights();
}

void UUtilityAIGoalComponent::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _RefreshWeights();
}

void UUtilityAIGoalComponent::PostEditUndo()
{
   Super::PostEditUndo();
   _RefreshWeights();
}
#endif // WITH_EDITOR

