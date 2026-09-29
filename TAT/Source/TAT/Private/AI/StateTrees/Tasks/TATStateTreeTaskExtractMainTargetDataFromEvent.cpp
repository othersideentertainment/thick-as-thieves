// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/TATStateTreeTaskExtractMainTargetDataFromEvent.h"

// ue
#include "StateTreeEvents.h"
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "AI/StateTrees/Targeting/TATStateTreeTargetingComponent.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskExtractMainTargetDataFromEvent)

EStateTreeRunStatus FTATStateTreeTaskExtractMainTargetDataFromEvent::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const TConstArrayView<FStateTreeSharedEvent> events = context.GetEventsToProcessView();
   if(events.Num() == 0)
      return EStateTreeRunStatus::Failed;

   const FStateTreeEvent& event = *events[0];
   if(event.Tag != TAG_StateTreeEvent_TargetChange)
      return EStateTreeRunStatus::Failed;

   const FTATAITargetingEvent_TargetChanged* targetChangeEventData = event.Payload.GetPtr<FTATAITargetingEvent_TargetChanged>();
   if(AActor** actorPtr = instanceData.ResultActor.GetMutablePtr<AActor*>(context))
   {
      *actorPtr = targetChangeEventData->Target;
      return EStateTreeRunStatus::Succeeded;
   }
   return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FTATStateTreeTaskExtractMainTargetDataFromTargetingGroup::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractMainTargetDataFromTargetingGroup failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractMainTargetDataFromTargetingGroup failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }
   
   if(const UTATStateTreeTargetingComponent* targetingComponent = aiController->GetStateTreeTargetingComponent())
   {
      AActor* target = targetingComponent->GetBestTargetForTargetingGroup(instanceData.TargetingGroup);
      if(AActor** actorPtr = instanceData.ResultActor.GetMutablePtr<AActor*>(context))
      {
         *actorPtr = target;
         return EStateTreeRunStatus::Succeeded;
      }
   }
   return EStateTreeRunStatus::Failed;
}
