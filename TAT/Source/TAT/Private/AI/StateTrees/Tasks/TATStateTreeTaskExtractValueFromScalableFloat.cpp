// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskExtractValueFromScalableFloat.h"

// ue
#include "StateTreeEvents.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskExtractValueFromScalableFloat)

EStateTreeRunStatus FTATStateTreeTaskExtractValueFromScalableFloat::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   double* floatPtr = instanceData.OutFloatValue.GetMutablePtr<double>(context);
   if (floatPtr == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractValueFromScalableFloat failed to find an out float param."));
      return EStateTreeRunStatus::Failed;
   }

   *floatPtr = instanceData.ScalableFloat.GetValueAtLevel(instanceData.Level);

   return EStateTreeRunStatus::Succeeded;
}
