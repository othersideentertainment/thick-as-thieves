// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "AI/StateTrees/Tasks/TATStateTreeTaskSetValue.h"
// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskSetValue)


EStateTreeRunStatus FTATStateTreeTaskSetValueBool::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   bool* boolPointer = instanceData.ValueToSet.GetMutablePtr<bool>(context);
   if(boolPointer == nullptr)
      return EStateTreeRunStatus::Failed;

   *boolPointer = instanceData.ValueToUse;
   return EStateTreeRunStatus::Succeeded;
}
