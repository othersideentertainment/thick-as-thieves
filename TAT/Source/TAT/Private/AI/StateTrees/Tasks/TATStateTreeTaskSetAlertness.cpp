// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskSetAlertness.h"

// ue
#include "StateTreeExecutionContext.h"
#include "Navigation/PathFollowingComponent.h"

// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskSetAlertness)

EStateTreeRunStatus FTATStateTreeTaskSetAlertness::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetAlertness failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }
   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetAlertness failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }
   if(UOSEAlertnessComponent* alertnessComponent = aiController->GetAlertnessComponent())
   {
      if(alertnessComponent->GetAlertnessLevel() < instanceData.AlertnessLevel)
      {
         alertnessComponent->AuthorityRaiseAlertnessLevelToAtLeast(instanceData.AlertnessLevel);
      }
      else
      {
         alertnessComponent->AuthorityLowerAlertnessLevelToAtMost(instanceData.AlertnessLevel);
      }
   }
   return instanceData.RunUntilAnimationFinished ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FTATStateTreeTaskSetAlertness::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetAlertness failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }
   if (UPathFollowingComponent* pathFollowingComponent = instanceData.AIController->GetPathFollowingComponent())
   {
      return pathFollowingComponent->IsResourceLocked() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
   }
   UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetAlertness failed since AIController has no path following component."));
   return EStateTreeRunStatus::Failed;
}
