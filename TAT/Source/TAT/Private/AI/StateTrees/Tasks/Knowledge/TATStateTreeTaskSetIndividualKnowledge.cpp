// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Knowledge/TATStateTreeTaskSetIndividualKnowledge.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATIndividualKnowledgeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskSetIndividualKnowledge)

EStateTreeRunStatus FTATStateTreeTaskSetIndividualKnowledge::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.Target == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetIndividualKnowledge failed since Target is missing."));
      return EStateTreeRunStatus::Failed;
   }
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetIndividualKnowledge failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetIndividualKnowledge failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }

   if(UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = aiController->GetIndividualKnowledgeComponent())
   {
      if(instanceData.ShouldAdd)
      {
         individualKnowledgeComponent->AddTag(instanceData.Target, instanceData.Tag, instanceData.TagShouldExpireIn);
      }
      else
      {
         individualKnowledgeComponent->RemoveTag(instanceData.Target, instanceData.Tag);
      }
      return EStateTreeRunStatus::Succeeded;
   }
   return EStateTreeRunStatus::Failed;
}
