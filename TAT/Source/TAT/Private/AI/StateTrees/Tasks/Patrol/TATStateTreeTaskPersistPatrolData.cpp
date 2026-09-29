// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Patrol/TATStateTreeTaskPersistPatrolData.h"

#include "AI/TATAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskPersistPatrolData)

FTATStateTreeTaskPersistPatrolData::FTATStateTreeTaskPersistPatrolData()
{
   bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FTATStateTreeTaskPersistPatrolData::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   return FStateTreeTaskCommonBase::EnterState(context, transition);
}

void FTATStateTreeTaskPersistPatrolData::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetBlackboardValue failed since AIController is missing."));
      return;
   }
   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetBlackboardValue failed since AIController is not a TATAIController"));
      return;
   }
   const APawn* pawn = aiController->GetPawn();
   if (pawn == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetBlackboardValue failed since the controller doesn't have a pawn"));
      return;
   }
   if(UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
   {
      blackboard->SetValueAsVector(ATATAIController::GetPatrolBrokenLocationBlackboardKey(), pawn->GetActorLocation());
   }
}
