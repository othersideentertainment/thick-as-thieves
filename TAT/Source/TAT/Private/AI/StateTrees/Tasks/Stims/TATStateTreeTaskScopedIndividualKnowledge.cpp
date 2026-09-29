// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Stims/TATStateTreeTaskScopedIndividualKnowledge.h"

// tat
#include "AI/TATAIController.h"

// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskScopedIndividualKnowledge)

bool FTATStateTreeTaskScopedIndividualKnowledge::SetIndividualKnowledge(const FStateTreeExecutionContext& context,
                                                                        const bool shouldAdd) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error,
              TEXT("FTATStateTreeTaskScopedIndividualKnowledge failed since AIController is missing."));
      return false;
   }
   if (instanceData.Target == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error,
              TEXT("FTATStateTreeTaskScopedIndividualKnowledge failed since Target is missing."));
      return false;
   }
   if (instanceData.KnowledgeTag.IsValid() == false)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error,
              TEXT("FTATStateTreeTaskScopedIndividualKnowledge failed since KnowledgeTag is invalid."));
      return false;
   }
   
   const ATATAIController* asTatAIController = Cast<ATATAIController>(instanceData.AIController);
   if(asTatAIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error,
              TEXT("FTATStateTreeTaskScopedIndividualKnowledge failed since AIController is incorrect type."));
      return false;
   }
   
   if (UOSEIndividualKnowledgeComponent* individualKnowledge = asTatAIController->GetIndividualKnowledgeComponent())
   {
      if (shouldAdd)
      {
         individualKnowledge->AddTag(instanceData.Target, instanceData.KnowledgeTag);
      }
      else
      {
         individualKnowledge->RemoveTag(instanceData.Target, instanceData.KnowledgeTag);
      }
   }
   return true;
}

EStateTreeRunStatus FTATStateTreeTaskScopedIndividualKnowledge::EnterState(FStateTreeExecutionContext& context,
                                                                      const FStateTreeTransitionResult& transition) const
{
   if (SetIndividualKnowledge(context, true))
   {
      return EStateTreeRunStatus::Running;
   }
   return EStateTreeRunStatus::Failed;
}

void FTATStateTreeTaskScopedIndividualKnowledge::ExitState(
   FStateTreeExecutionContext& context,
   const FStateTreeTransitionResult& transition) const
{
   const bool didRemoveKnowledge = SetIndividualKnowledge(context, false);
}
