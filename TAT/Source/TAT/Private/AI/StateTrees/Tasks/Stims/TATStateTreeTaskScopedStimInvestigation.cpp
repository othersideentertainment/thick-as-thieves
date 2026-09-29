// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Stims/TATStateTreeTaskScopedStimInvestigation.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATKnowledgeComponent.h"

// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskScopedStimInvestigation)

EStateTreeRunStatus FTATStateTreeTaskScopedStimInvestigation::EnterState(FStateTreeExecutionContext& context,
                                                                         const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (TrySetInvestigationState(context.GetOwner(), instanceData, EStimInvestigationState::UnderInvestigation))
   {
      return EStateTreeRunStatus::Running;
   }
   return EStateTreeRunStatus::Failed;
}

bool FTATStateTreeTaskScopedStimInvestigation::TrySetInvestigationState(
   const UObject* owner,
   const FInstanceDataType& instanceData,
   const EStimInvestigationState investigationState)
{
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(owner, LogStateTree, Error, TEXT("FTATStateTreeTaskScopedStimInvestigation::TrySetInvestigationState failed since AIController is missing."));
      return false;
   }
   const ATATAIController* asTatAIController = Cast<ATATAIController>(instanceData.AIController);
   if(asTatAIController == nullptr)
   {
      UE_VLOG(owner, LogStateTree, Error, TEXT("FTATStateTreeTaskScopedStimInvestigation::TrySetInvestigationState failed since AIController is incorrect type."));
      return false;
   }
   UTATKnowledgeComponent* knowledge = asTatAIController->GetTATKnowledgeComponent();
   knowledge->SetStimInvestigationState(instanceData.StimID, investigationState);
   return true;
}

void FTATStateTreeTaskScopedStimInvestigation::ExitState(
   FStateTreeExecutionContext& context,
   const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   TrySetInvestigationState(context.GetOwner(), instanceData, EStimInvestigationState::Investigated);   
}
