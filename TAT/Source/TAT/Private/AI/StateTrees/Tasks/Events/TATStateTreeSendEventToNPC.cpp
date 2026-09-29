// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "AI/StateTrees/Tasks/Events/TATStateTreeSendEventToNPC.h"
#include "AI/TATAIController.h"

// ose
#include "Character/OSECharacterBase.h"

// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeSendEventToNPC)

namespace TATStateTreeSendEventHelpers
{
   static const ATATAIController* TryGetController(const AActor* actor)
   {
      if (const AOSECharacterBase* aiCharacter = Cast<AOSECharacterBase>(actor))
      {
         return Cast<ATATAIController>(aiCharacter->GetController());
      }

      return Cast<ATATAIController>(actor);
   }
}

FInstancedStruct FTATStateTreeSendEventToNPC::GetEventPayload(FStateTreeExecutionContext& context, ATATAIController* aiController) const
{
   return FInstancedStruct::Make(FTATStateTreeInterNPCEvent({aiController->GetPawn()}));
}

EStateTreeRunStatus FTATStateTreeSendEventToNPC::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if(aiController == nullptr)
      return EStateTreeRunStatus::Failed;

   const ATATAIController* targetCharacterAIController = TATStateTreeSendEventHelpers::TryGetController(instanceData.TargetNPC);
   if(targetCharacterAIController == nullptr)
      return EStateTreeRunStatus::Failed;
   
   UStateTreeComponent* stateTreeComponent = Cast<UStateTreeComponent>(targetCharacterAIController->GetBrainComponent());
   if(stateTreeComponent == nullptr)
      return EStateTreeRunStatus::Failed;
   
   FStateTreeEvent eventToSend;
   eventToSend.Payload = GetEventPayload(context, aiController);
   eventToSend.Tag = instanceData.EventTagToUse;
   stateTreeComponent->SendStateTreeEvent(eventToSend);
   return EStateTreeRunStatus::Succeeded;
}

FInstancedStruct FTATStateTreeSendEventToNPC_VoiceLine::GetEventPayload(FStateTreeExecutionContext& context, ATATAIController* aiController) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   FTATStateTreeInterNPCEvent_VoiceLine returnStruct;
   returnStruct.FromActor = aiController->GetPawn();
   returnStruct.ResponseVoLine = instanceData.VoiceLineToPlay;
   returnStruct.PreDelayTime = instanceData.DelayBeforeVoiceLine;
   returnStruct.PostDelayTime = instanceData.DelayAfterVoiceLine;
   return FInstancedStruct::Make(returnStruct);
}

FInstancedStruct FTATStateTreeSendEventToNPC_ShareTarget::GetEventPayload(FStateTreeExecutionContext& context, ATATAIController* aiController) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   FTATStateTreeInterNPCEvent_ShareTarget returnStruct;
   returnStruct.FromActor = aiController->GetPawn();
   returnStruct.ResponseVoLine = instanceData.VoiceLineToPlay;
   returnStruct.PreDelayTime = instanceData.DelayBeforeVoiceLine;
   returnStruct.PostDelayTime = instanceData.DelayAfterVoiceLine;
   returnStruct.SharedTargetActor = instanceData.TargetToShare;
   returnStruct.SharedTargetLocation = instanceData.SharedTargetLocation;
   return FInstancedStruct::Make(returnStruct);
}

EStateTreeRunStatus FTATStateTreeExtractEventToNPC_VoiceLine::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const TConstArrayView<FStateTreeSharedEvent> events = context.GetEventsToProcessView();
   if(events.Num() == 0)
      return EStateTreeRunStatus::Failed;

   const FStateTreeEvent& event = *events[0].Get();
   const FTATStateTreeInterNPCEvent_VoiceLine* voiceLineData = event.Payload.GetPtr<FTATStateTreeInterNPCEvent_VoiceLine>();

   if(voiceLineData == nullptr)
      return EStateTreeRunStatus::Failed;
   double* preDelayPtr = instanceData.DelayBeforeVoice.GetMutablePtr<double>(context);
   double* postDelayPtr = instanceData.DelayAfterVoice.GetMutablePtr<double>(context);
   FGameplayTag* voLinePtr = instanceData.VOLineToPlay.GetMutablePtr<FGameplayTag>(context);
   AActor** actorPtr = instanceData.FromActor.GetMutablePtr<AActor*>(context);
   if(postDelayPtr == nullptr || preDelayPtr == nullptr || voLinePtr == nullptr || actorPtr == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   
   *actorPtr = voiceLineData->FromActor;
   *preDelayPtr = voiceLineData->PreDelayTime;
   *postDelayPtr = voiceLineData->PostDelayTime;
   *voLinePtr = voiceLineData->ResponseVoLine;

   return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FTATStateTreeExtractEventToNPC_ShareTarget::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const EStateTreeRunStatus returnStatus = FTATStateTreeExtractEventToNPC_VoiceLine::EnterState(context, transition);
   if(returnStatus != EStateTreeRunStatus::Succeeded)
      return returnStatus;

   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const TConstArrayView<FStateTreeSharedEvent> events = context.GetEventsToProcessView();
   if(events.Num() == 0)
      return EStateTreeRunStatus::Failed;

   const FStateTreeEvent& event = *events[0].Get();
   const FTATStateTreeInterNPCEvent_ShareTarget* shareTarget = event.Payload.GetPtr<FTATStateTreeInterNPCEvent_ShareTarget>();
   AActor** actorPtr = instanceData.SharedTargetActor.GetMutablePtr<AActor*>(context);
   FVector* vectorPtr = instanceData.SharedLocationVector.GetMutablePtr<FVector>(context);
   if(actorPtr == nullptr && vectorPtr == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   if(actorPtr != nullptr)
   {
      *actorPtr = shareTarget->SharedTargetActor;
   }
   if(vectorPtr != nullptr)
   {
      *vectorPtr = shareTarget->SharedTargetLocation;
   }
   return EStateTreeRunStatus::Succeeded;
}
