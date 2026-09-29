// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/OSEAISense_Team.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAISense_Team)

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
FOSEAITeamStimulusEvent::FOSEAITeamStimulusEvent(AActor* inBroadcaster, AActor* inTarget, const FVector& inLastKnowLocation, float eventRange, float passedInfoAge, float inStrength)
   : LastKnowLocation(inLastKnowLocation), RangeSq(FMath::Square(eventRange)), InformationAge(passedInfoAge), Strength(inStrength), Broadcaster(inBroadcaster), Target(inTarget)
{
   CacheBroadcastLocation();

   TeamAttitude = UOSETeamFunctionLibrary::GetTeamAttitude(inBroadcaster, inTarget);
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
UOSEAISense_Team::UOSEAISense_Team(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

float UOSEAISense_Team::Update()
{
   AIPerception::FListenerMap& listenersMap = *GetListeners();
   
   for (AIPerception::FListenerMap::TIterator listenerIt(listenersMap); listenerIt; ++listenerIt)
   {
      FPerceptionListener& listener = listenerIt->Value;

      if (listener.HasSense(GetSenseID()) == false)
      {
         // skip listeners not interested in this sense
         continue;
      }

      for (const FOSEAITeamStimulusEvent& event : RegisteredEvents)
      {
         const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(listener.GetBodyActor(), event.Target);
         // @todo implement some kind of TeamIdentifierType that would supply comparison operator 
         if (attitude != event.TeamAttitude 
            || FVector::DistSquared(event.GetBroadcastLocation(), listener.CachedLocation) > event.RangeSq)
         {
            continue;
         }
         
         listener.RegisterStimulus(event.Target, FAIStimulus(*this, event.Strength, event.LastKnowLocation, event.GetBroadcastLocation(), FAIStimulus::SensingSucceeded).SetStimulusAge(event.InformationAge));
      }
   }

   RegisteredEvents.Reset();

   // return decides when next tick is going to happen
   return SuspendNextUpdate;
}

void UOSEAISense_Team::RegisterEvent(const FOSEAITeamStimulusEvent& inEvent)
{
   RegisteredEvents.Add(inEvent);

   RequestImmediateUpdate();
}

