// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "AI/StateTrees/Tasks/TATStateTreeTaskLoop.h"
// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskLoop)

EStateTreeRunStatus FTATStateTreeTaskLoop::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   int32* loopCountPointer = instanceData.LoopCount.GetMutablePtr<int32>(context);
   if(loopCountPointer == nullptr)
      return EStateTreeRunStatus::Failed;
   
   (*loopCountPointer)++;
   if((*loopCountPointer) >= instanceData.MaxLoopCount)
   {
      *loopCountPointer = 0;
      return EStateTreeRunStatus::Failed;
   }
   return EStateTreeRunStatus::Succeeded;
}
