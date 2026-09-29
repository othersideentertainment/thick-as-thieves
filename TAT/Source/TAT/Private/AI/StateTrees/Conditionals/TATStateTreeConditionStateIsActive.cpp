// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Conditionals/TATStateTreeConditionStateIsActive.h"

// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionStateIsActive)

bool FTATStateTreeConditionStateIsActive::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   // NOTE : FStateTreeStateLink's StateHandle is only unique in the context
   // of the frame it exists within. Make sure do not try to compare it to
   // handles in other frames (i.e. other state tree assets).

   // The frame that this conditional is being used within. SHould exist
   // if this method is being called.
   const FStateTreeExecutionFrame* processingFrame = context.GetCurrentlyProcessedFrame();
   check(processingFrame != nullptr);
   
   // If a state has been linked with us, it must be from the same ST asset.
   const bool stateIsActive = processingFrame->ActiveStates.Contains(instanceData.State.StateHandle);
   return (stateIsActive == instanceData.SucceedIfActive);
}
