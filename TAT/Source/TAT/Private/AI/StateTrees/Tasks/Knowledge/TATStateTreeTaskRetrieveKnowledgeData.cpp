// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/Knowledge/TATStateTreeTaskRetrieveKnowledgeData.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATKnowledgeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskRetrieveKnowledgeData)

EStateTreeRunStatus FTATStateTreeTaskRetrieveShareTarget::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRetrieveShareTarget failed since AIController is not a TATAIController."));
      return EStateTreeRunStatus::Failed;
   }

   const UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   if (knowledge == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRetrieveShareTarget failed to find the AI's knowledge component."));
      return EStateTreeRunStatus::Failed;
   }

   const FTATSharedTarget& shareTarget = knowledge->GetTargetToShare();
   if (!ensure(shareTarget.IsValid()))
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRetrieveShareTarget used when AI didn't have a valid share target."));
      return EStateTreeRunStatus::Failed;
   }

   bool foundOutParam = false;
   AActor* shareTargetActor = shareTarget.TargetActor.Get();
   if (AActor** outActorPtr = instanceData.OutShareTargetActor.GetMutablePtr<AActor*>(context))
   {
      *outActorPtr = shareTargetActor;
      foundOutParam = true;
   }
   if (FVector* outVectorPtr = instanceData.OutShareTargetLocation.GetMutablePtr<FVector>(context))
   {
      ensure(knowledge == shareTarget.Instigator);

	  if (knowledge->GetLastKnownActorLocation(shareTargetActor, *outVectorPtr))
	  {
		  // nothing necessary, the location has been retrieved
	  }
      else if (const FStimInfo* targetStimInfo = knowledge->GetStimInfo(shareTarget.TargetStimId))
      {
         *outVectorPtr = targetStimInfo->Location;
      }
      else if (shareTarget.TargetLocation.IsSet())
      {
         *outVectorPtr = shareTarget.TargetLocation.GetValue();
      }

      foundOutParam = true;
   }
   ensureMsgf(foundOutParam, TEXT("FTATStateTreeTaskRetrieveShareTarget was not supplied an out param."));

   return EStateTreeRunStatus::Succeeded;
}
