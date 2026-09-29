// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Conditionals/GameplayTags/TATStateTreeConditionGameplayTagIsValid.h"

// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionGameplayTagIsValid)

bool FTATStateTreeConditionGameplayTagIsValid::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   return instanceData.SucceedIfValid == instanceData.TagToCheck.IsValid();
}
