// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


// tat
#include "AI/StateTrees/Tasks/Movement/TATStateTreeTaskMoveTo.h"

// ue
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "Tasks/AITask_MoveTo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskMoveTo)


UAITask_MoveTo* FTATStateTreeMoveToTask::PrepareMoveToTask(
   FStateTreeExecutionContext& context,
   AAIController& controller,
   UAITask_MoveTo* existingTask,
   FAIMoveRequest& moveRequest) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   UAITask_MoveTo* moveToTask = FStateTreeMoveToTask::PrepareMoveToTask(context, controller, existingTask, moveRequest); 
   if (moveToTask)
   {
      moveToTask->SetContinuousGoalTracking(instanceData.bContinuousGoalTracking);
      moveToTask->GetMoveRequestRef().SetUsePathfinding(instanceData.bShouldDirectMove == false);
#if ENABLE_VISUAL_LOG
      UE_VLOG_SPHERE(
         &controller,
         LogTemp,
         Log,
         moveToTask->GetMoveRequestRef().GetDestination(),
         32.f,
         FColor::Green,
         TEXT("")
      );
#endif
   }
   return moveToTask;
}
