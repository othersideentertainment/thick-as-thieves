// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Patrol/Distractions/TATStateTreeTaskExtractDistractionEvent.h"

// tat
#include "AI/StateTrees/TATStateTreeEvents.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskExtractDistractionEvent)

EStateTreeRunStatus FTATStateTreeTaskExtractDistractionEvent::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const TConstArrayView<FStateTreeSharedEvent> events = context.GetEventsToProcessView();
   FSmartObjectSlotHandle* slotHandle = instanceData.SmartObjectHandle.GetMutablePtr<FSmartObjectSlotHandle>(context);

   if(slotHandle == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractDistractionEvent failed since SlotHandle is missing."));
      return EStateTreeRunStatus::Failed;
   }
   for (const FStateTreeFrameStateSelectionEvents& frameEvents : transition.NextActiveFrameEvents)
   {
      for (const FStateTreeSharedEvent& event : frameEvents.Events)
      {
         if(event.IsValid() == false)
            continue;
         
         // We only want to process distraction events.
         if (event->Tag != TAG_StateTreeEvent_DistractionEvent)
            continue;
         
         const FTATAITargetingEvent_DistractionEvent* distractionEvent = event->Payload.GetPtr<FTATAITargetingEvent_DistractionEvent>();
         *slotHandle = distractionEvent->DistractionHandle;
         return EStateTreeRunStatus::Succeeded;
      }
   }
   
   return EStateTreeRunStatus::Failed;
}
