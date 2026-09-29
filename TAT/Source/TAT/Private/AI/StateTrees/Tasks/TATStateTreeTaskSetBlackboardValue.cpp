// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskSetBlackboardValue.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskSetBlackboardValue)

EStateTreeRunStatus FTATStateTreeTaskSetBlackboardValue::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetBlackboardValue failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }
   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetBlackboardValue failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }
   if(UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
   {
      blackboard->SetValueAsVector(instanceData.BlackboardKeyName, instanceData.Location);
      return EStateTreeRunStatus::Succeeded;
   }
   return FStateTreeTaskCommonBase::EnterState(context, transition);
}

EStateTreeRunStatus FTATStateTreeTaskSetBlackboardActorValue::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetBlackboardActorValue failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }
   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetBlackboardActorValue failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }
   if(UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
   {
      blackboard->SetValueAsObject(instanceData.BlackboardKeyName, instanceData.Target);
      return EStateTreeRunStatus::Succeeded;
   }
   return EStateTreeRunStatus::Failed;
}
