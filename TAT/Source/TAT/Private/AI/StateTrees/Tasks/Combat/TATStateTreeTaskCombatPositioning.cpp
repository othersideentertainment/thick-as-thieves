// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Combat/TATStateTreeTaskCombatPositioning.h"

#include "StateTreeExecutionContext.h"
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskCombatPositioning)

FTATStateTreeTaskCombatPositioning::FTATStateTreeTaskCombatPositioning()
{
   bShouldStateChangeOnReselect = false;
   bShouldCallTick = false;
}

bool FTATStateTreeTaskCombatPositioning::HandleSetCombatPosition(
   const FStateTreeExecutionContext& context,
   const bool shouldReset) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskCombatPositioning failed since AIController is missing."));
      return false;
   }

   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskCombatPositioning failed since AIController is not a TATAIController"));
      return false;
   }
   if (shouldReset)
   {
      aiController->SetRequestedCombatPosition(instanceData.PreviousCombatPosition);
   }
   else
   {
      instanceData.PreviousCombatPosition = aiController->GetRequestedCombatPosition();
      aiController->SetRequestedCombatPosition(instanceData.CombatPosition);
   }
   return true;
}

EStateTreeRunStatus FTATStateTreeTaskCombatPositioning::EnterState(FStateTreeExecutionContext& context,
                                                                   const FStateTreeTransitionResult& transition) const
{
   if (HandleSetCombatPosition(context, false) == false)
      return EStateTreeRunStatus::Failed;
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskCombatPositioning::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   // ReSharper disable once CppExpressionWithoutSideEffects
   HandleSetCombatPosition(context, true);
}
