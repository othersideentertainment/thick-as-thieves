// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/AISense_VisualEvent.h"

#include "AI/Perception/AISenseEvent_VisualEvent.h"
#include "AI/Perception/OSEAISense_Sight.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AISense_VisualEvent)

FAIVisualEvent::FAIVisualEvent(AActor* instigator, FName tag)
   : Instigator(instigator)
   , Tag(tag)
{
   Compile();
}

void FAIVisualEvent::Compile()
{
   TeamIdentifier = FGenericTeamId::GetTeamIdentifier(Instigator);
}

void UAISense_VisualEvent::ReportVisualEvent(UObject* worldContextObject, AActor* instigator, FName tag)
{
   if (!instigator)
   {
      UE_LOG(LogAIPerception, Warning, TEXT("UAISense_VisualEvent::ReportVisualEvent requires a valid instigator!"));
      return;
   }

   UAIPerceptionSystem* perceptionSystem = UAIPerceptionSystem::GetCurrent(worldContextObject);
   if (perceptionSystem)
   {
      FAIVisualEvent event(instigator, tag);
      perceptionSystem->OnEvent(event);
   }
}

void UAISense_VisualEvent::RegisterEvent(const FAIVisualEvent& event)
{
   VisualEvents.Add(event);

   RequestImmediateUpdate();
}

void UAISense_VisualEvent::RegisterEventsBatch(const TArray<FAIVisualEvent>& events)
{
   VisualEvents.Append(events);

   RequestImmediateUpdate();
}

void UAISense_VisualEvent::RegisterWrappedEvent(UAISenseEvent& perceptionEvent)
{
   UAISenseEvent_VisualEvent* sightEvent = Cast<UAISenseEvent_VisualEvent>(&perceptionEvent);
   ensure(sightEvent);
   if (sightEvent)
   {
      RegisterEvent(sightEvent->GetSightEvent());
   }
}

float UAISense_VisualEvent::Update()
{
   AIPerception::FListenerMap& listenersMap = *GetListeners();
   UAIPerceptionSystem* perseptionSys = GetPerceptionSystem();

   const FAISenseID sightSenseID = UAISense::GetSenseID<UOSEAISense_Sight>();

   for (AIPerception::FListenerMap::TIterator listenerIt(listenersMap); listenerIt; ++listenerIt)
   {
      FPerceptionListener& listener = listenerIt->Value;
      
      if (!listener.HasSense(GetSenseID()) || !listener.HasSense(sightSenseID))
      {
         // Listeners must have both this and Sight sense.
         continue;
      }

      for (const FAIVisualEvent& event : VisualEvents)
      {
         if (!event.Instigator)
         {
            continue;
         }

         // Check if this listener currently sees the instigator, to avoid having to recast.
         const FActorPerceptionInfo* perceivedInfo = listener.Listener->GetActorInfo(*event.Instigator);
         if (!perceivedInfo || !perceivedInfo->IsSenseActive(sightSenseID))
         {
            // This listener can't currently see the Instigator, so ignore it.
            continue;
         }

         // TODO: Compute some sort of stim strength?
         const float stimulusStrength = 1.0f;
         listener.RegisterStimulus(event.Instigator, FAIStimulus(*this, stimulusStrength, perceivedInfo->GetStimulusLocation(sightSenseID), listener.CachedLocation, FAIStimulus::SensingSucceeded, event.Tag));
      }
   }

   VisualEvents.Reset();

   // Only run when new events are added.
   return SuspendNextUpdate;
}

