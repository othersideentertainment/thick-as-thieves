// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


// tat
#include "AI/StateTrees/Tasks/Stims/TATStateTreeTaskUpdateStimInfo.h"

#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskUpdateStimInfo)

EStateTreeRunStatus FTATStateTreeTaskUpdateStimInfo::EnterState(FStateTreeExecutionContext& context,
                                                                const FStateTreeTransitionResult& transition) const
{
   return HandleUpdate(context);
}

EStateTreeRunStatus FTATStateTreeTaskUpdateStimInfo::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   return HandleUpdate(context);
}

EStateTreeRunStatus FTATStateTreeTaskUpdateStimInfo::HandleUpdate(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskUpdateStimInfo failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }
   const ATATAIController* asTatAIController = Cast<ATATAIController>(instanceData.AIController);
   if(asTatAIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskUpdateStimInfo failed since AIController is incorrect type."));
      return EStateTreeRunStatus::Failed;
   }
   const UTATKnowledgeComponent* knowledge = asTatAIController->GetTATKnowledgeComponent();
   if(const FStimInfo* stimInfo = knowledge->GetStimInfo(instanceData.StimID))
   {
      if(FVector* vectorPtr = instanceData.OutStimInfoLocation.GetMutablePtr<FVector>(context))
      {
         *vectorPtr = stimInfo->Location;
         return EStateTreeRunStatus::Running;
      }
   }
   else
   {
      // If the stim is expired, still complete the behavior
      return EStateTreeRunStatus::Running;
   }
   return EStateTreeRunStatus::Failed;
}
