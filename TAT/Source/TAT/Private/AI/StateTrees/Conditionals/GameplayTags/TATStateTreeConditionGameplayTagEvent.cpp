// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/GameplayTags/TATStateTreeConditionGameplayTagEvent.h"

// ue
#include "StateTreeEvents.h"
#include "StateTreeExecutionContext.h"

// tat
#include "AI/StateTrees/TATStateTreeEvents.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionGameplayTagEvent)

namespace TATStateTreeConditionGameplayTagEvent
{
   const FTATAITargetingEvent_GameplayTagChanged* GetEventFromContext(const FStateTreeExecutionContext& context)
   {
      const TConstArrayView<FStateTreeSharedEvent> events = context.GetEventsToProcessView();
      for (const FStateTreeSharedEvent& event : events)
      {
         if(event->Tag != TAG_StateTreeEvent_GameplayTagChange)
            continue;
         return event->Payload.GetPtr<FTATAITargetingEvent_GameplayTagChanged>();
      }
      return nullptr;
   }
}
bool FTATStateTreeConditionGameplayTagEvent::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const FTATAITargetingEvent_GameplayTagChanged* tagEvent = TATStateTreeConditionGameplayTagEvent::GetEventFromContext(context);
   if(tagEvent == nullptr)
      return false;

   if(tagEvent->Tag == instanceData.TagToMatch)
   {
      if(tagEvent->Exists == instanceData.RequireExisting)
      {
         return true;
      }
   }
   return false;
}
