// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/Reactions/TATStateTreeTaskExtractRoleFromEvent.h"

// ue
#include "StateTreeEvents.h"
#include "StateTreeExecutionContext.h"

// tat
#include "AI/StateTrees/TATStateTreeEvents.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskExtractRoleFromEvent)

EStateTreeRunStatus FTATStateTreeTaskExtractRoleFromEvent::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   FGameplayTag* outRoleTag = instanceData.ResultRegisteredRoleTag.GetMutablePtr<FGameplayTag>(context);
   if (outRoleTag == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractRoleFromEvent failed to find an out role tag param."));
      return EStateTreeRunStatus::Failed;
   }

   *outRoleTag = FGameplayTag::EmptyTag;
   for (const FStateTreeFrameStateSelectionEvents& nextActiveFrameEvent : transition.NextActiveFrameEvents)
   {
      for (const FStateTreeSharedEvent& stateTreeEvent : nextActiveFrameEvent.Events)
      {
         if(stateTreeEvent.IsValid())
         {
            if (stateTreeEvent->Tag != TAG_StateTreeEvent_ReactionRoleChange)
            {
               continue;
            }
            
            if (const FTATAITargetingEvent_ReactionRoleChanged* roleEvent = stateTreeEvent->Payload.GetPtr<FTATAITargetingEvent_ReactionRoleChanged>())
            {
               *outRoleTag = roleEvent->NewRoleTag;
               return EStateTreeRunStatus::Succeeded;
            }
         }
      }
   }

   UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractRoleFromEvent failed to find a reaction role change event."));
   return EStateTreeRunStatus::Failed;
}
