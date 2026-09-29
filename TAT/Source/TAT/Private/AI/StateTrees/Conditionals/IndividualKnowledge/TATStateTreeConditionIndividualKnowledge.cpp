// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/IndividualKnowledge/TATStateTreeConditionIndividualKnowledge.h"

// ue
#include "StateTreeExecutionContext.h"

// ose
#include "OSEIndividualKnowledgeBlueprintFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionIndividualKnowledge)

bool FTATStateTreeConditionIndividualKnowledge::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if(instanceData.Target == nullptr)
      return false;

   AAIController* aiController = instanceData.Controller;
   if(aiController == nullptr)
      return false;

   const bool hasTag = UOSEIndividualKnowledgeBlueprintFunctionLibrary::HasIndividualKnowledge(aiController, instanceData.Target, KnowledgeTagToCheck);
   return Invert == false ? hasTag : !hasTag;
}
