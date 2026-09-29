// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/MatchTimedEvents/TATMatchTimedEventComponent.h"

// tat
#include "Online/TATGameState.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchTimedEventComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATMatchTimedEventComponent, Log, All);

UTATMatchTimedEventComponent::UTATMatchTimedEventComponent()
{
   bAutoActivate = true;
   PrimaryComponentTick.bCanEverTick = false;
   SetIsReplicatedByDefault(true);
}

#if WITH_EDITOR
EDataValidationResult UTATMatchTimedEventComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);
   
   // Validate configured events
   for (const FTATMatchTimedEvent& timedEvent : MatchedTimedEvents)
   {
      // Ensure each event has valid EventId
      if (!timedEvent.EventId.IsValid())
      {
         context.AddError(FText::FromString(TEXT("Found event in MatchTimedEvents with invalid EventId!")));
      }
   }

   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}

void UTATMatchTimedEventComponent::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   if (propertyChangedEvent.MemberProperty)
   {
      const FName propName = propertyChangedEvent.Property->GetFName();
      static const FName NAME_MatchedTimedEvents = GET_MEMBER_NAME_CHECKED(ThisClass, MatchedTimedEvents);

      // Ensure all events use our DurationSupported value
      if (propName == NAME_MatchedTimedEvents)
      {
         for (FTATMatchTimedEvent& matchTimedEvent : MatchedTimedEvents)
         {
            matchTimedEvent.ExposeDuration = _durationSupported;
         }
      }
   }
}
#endif // WITH_EDITOR

void UTATMatchTimedEventComponent::PostLoad()
{
   Super::PostLoad();

   // Ensure all events use our DurationSupported value, in case any were serialized with wrong value
   for (FTATMatchTimedEvent& matchTimedEvent : MatchedTimedEvents)
   {
      matchTimedEvent.ExposeDuration = _durationSupported;
   }
}

void UTATMatchTimedEventComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ThisClass, _replicatedEventData);
}

void UTATMatchTimedEventComponent::Activate(bool reset)
{
   Super::Activate(reset);

   if (GetWorld()->IsGameWorld())
   {
      if (GetOwner()->HasAuthority())
      {
         ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>();
         check(tatGS);
         tatGS->CallOrRegisterMatchStartDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATMatchTimedEventComponent::_AuthorityOnMatchStart));
      }
   }
}

void UTATMatchTimedEventComponent::Deactivate()
{
   if (GetOwner()->HasAuthority())
   {
      // Kill any remaining event timers
      FTimerManager& timerManager = GetWorld()->GetTimerManager();
      for (FTATMatchTimedEvent& matchTimedEvent : MatchedTimedEvents)
      {
         if (matchTimedEvent.AuthorityTimerHandle.IsValid())
         {
            timerManager.ClearTimer(matchTimedEvent.AuthorityTimerHandle);
         }
      }

      if (ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>())
      {
         tatGS->OnPhaseTimerChanged.RemoveAll(this);
      }
   }
   Super::Deactivate();
}

float UTATMatchTimedEventComponent::GetTimeSecondsUntilEvent(FGameplayTag eventId, bool& found) const
{
   if (const FTATMatchTimedEvent* timedEvent = GetMatchTimedEvent(eventId))
   {
      found = true;
      const ATATGameState* tatGs = GetWorld()->GetGameState<ATATGameState>();
      if (tatGs && tatGs->HasTATMatchStarted())
      {
         return _GetSecondsUntilEvent(*timedEvent);
      }
      else
      {
         UE_LOG(LogTATMatchTimedEventComponent, Error, TEXT("GetTimeSecondsUntilEvent() called before match timer began!"));
         return -1.f;
      }
   }
   UE_LOG(LogTATMatchTimedEventComponent, Error, TEXT("[%s] GetSecondsUntilEvent() | could not find event with EventId = %s!"), *GetOwner()->GetName(), *eventId.ToString());
   found = false;
   return -1.f;
}

float UTATMatchTimedEventComponent::GetDurationSecondsRemainingForEvent(FGameplayTag eventId, bool& found) const
{
   if (const FTATMatchTimedEvent* timedEvent = GetMatchTimedEvent(eventId))
   {
      found = true;
      const ATATGameState* tatGs = GetWorld()->GetGameState<ATATGameState>();
      if (tatGs && tatGs->HasTATMatchStarted())
      {
         return _GetDurationRemainingForEvent(*timedEvent);
      }
      else
      {
         UE_LOG(LogTATMatchTimedEventComponent, Error, TEXT("GetDurationSecondsRemainingForEvent() called before match timer began!"));
         return -1.f;
      }
   }
   UE_LOG(LogTATMatchTimedEventComponent, Error, TEXT("[%s] GetDurationSecondsRemainingForEvent() | could not find event with EventId = %s!"), *GetOwner()->GetName(), *eventId.ToString());
   found = false;
   return -1.f;
}

void UTATMatchTimedEventComponent::SetDurationSupportedByDefault(bool durationSupported)
{
   check(FUObjectThreadContext::Get().IsInConstructor);
   _durationSupported = durationSupported;
}

void UTATMatchTimedEventComponent::_AuthorityOnMatchStart()
{
   check(GetOwner()->HasAuthority());

   ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>();
   check(tatGS);

   // Listen for match timer assignment (and subsequent updates)
   _AuthorityOnPhaseTimeChanged(tatGS->GetCurrentPhase());
   tatGS->OnPhaseTimerChanged.AddDynamic(this, &UTATMatchTimedEventComponent::_AuthorityOnPhaseTimeChanged);
}

void UTATMatchTimedEventComponent::_AuthorityOnPhaseTimeChanged(ETATMatchPhase phase)
{
   check(GetOwner()->HasAuthority());

   const ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>();
   check(tatGS);
   const float secondsUntilPhaseEnd = tatGS->GetTimeLeftInPhase();
   UE_LOG(LogTATMatchTimedEventComponent, Verbose, TEXT("[%s] Phase timer changed - ending in %f seconds"), *GetOwner()->GetName(), secondsUntilPhaseEnd);

   for (FTATMatchTimedEvent& timedEvent : MatchedTimedEvents)
   {
      if (!timedEvent.EventId.IsValid())
      {
         UE_LOG(LogTATMatchTimedEventComponent, Error, TEXT("[%s] _AuthorityOnMatchEndTimeChanged() detected event with invalid EventId!"), *GetOwner()->GetName());
         continue;
      }

      if (timedEvent.AuthorityEventState == ETATMatchTimedEventState::PendingEvent)
      {
         // Clear timer
         GetWorld()->GetTimerManager().ClearTimer(timedEvent.AuthorityTimerHandle);

         // Check new time-until-event, resetting timer or triggering event accordingly
         const float secondsUntilEvent = _GetSecondsUntilEvent(timedEvent);
         if (secondsUntilEvent <= 0)
         {
            // Handle any events that occurred in the "past" (i.e. in the time skipped by a match timer decrement)
            const float timeElapsedSinceOccurred = FMath::Abs(secondsUntilEvent);
            _AuthorityOnEventOccurred(timedEvent, timeElapsedSinceOccurred);
         }
         else
         {
            // Set new timer for event
            _AuthoritySetTimerForEvent(timedEvent, secondsUntilEvent);
         }
      }
   }
}

void UTATMatchTimedEventComponent::_AuthorityOnEventOccurred(FTATMatchTimedEvent& timedEvent, float timeElapsedSinceOccurred)
{
   check(GetOwner()->HasAuthority());
   check(timedEvent.AuthorityEventState != ETATMatchTimedEventState::Elapsed);

   switch (timedEvent.AuthorityEventState)
   {
   case ETATMatchTimedEventState::PendingEvent:
      if (timeElapsedSinceOccurred > 0)
      {
         UE_LOG(LogTATMatchTimedEventComponent, Verbose, TEXT("[%s] %s occurred %f seconds ago"), *GetOwner()->GetName(), *timedEvent.ToString(), timeElapsedSinceOccurred);
      }
      else
      {
         UE_LOG(LogTATMatchTimedEventComponent, Verbose, TEXT("[%s] %s occurred"), *GetOwner()->GetName(), *timedEvent.ToString());
      }

      // Notify listeners
      AuthorityOnTimedEventOccurred.Broadcast(timedEvent.EventId, timeElapsedSinceOccurred);

      if (timedEvent.HasDuration())
      {
         // Set timer for duration
         timedEvent.AuthorityEventState = ETATMatchTimedEventState::PendingDuration;
         _AuthoritySetTimerForEvent(timedEvent, timedEvent.Duration);
      }
      else
      {
         timedEvent.AuthorityEventState = ETATMatchTimedEventState::Elapsed;
      }

      // Replicate event trigger time to clients
      check(!_replicatedEventData.Contains(timedEvent));
      _replicatedEventData.Emplace(timedEvent, GetWorld()->GetTimeSeconds());
      break;

   case ETATMatchTimedEventState::PendingDuration:
      check(timedEvent.HasDuration());
      UE_LOG(LogTATMatchTimedEventComponent, Verbose, TEXT("[%s] %s duration elapsed"), *GetOwner()->GetName(), *timedEvent.ToString());

      // Notify listeners and mark event as elapsed
      AuthorityOnTimedEventDurationElapsed.Broadcast(timedEvent.EventId);
      timedEvent.AuthorityEventState = ETATMatchTimedEventState::Elapsed;
      break;

   default:
      checkNoEntry();
   }
}

void UTATMatchTimedEventComponent::_AuthoritySetTimerForEvent(FTATMatchTimedEvent& timedEvent, float secondsUntilActivation)
{
   check(GetOwner()->HasAuthority());
   check(timedEvent.AuthorityEventState != ETATMatchTimedEventState::Elapsed);
   UE_LOG(LogTATMatchTimedEventComponent, Verbose, TEXT("Setting %f second %s for event: %s")
      , secondsUntilActivation
      , timedEvent.AuthorityEventState == ETATMatchTimedEventState::PendingDuration ? TEXT("duration") : TEXT("timer")
      , *timedEvent.ToString());
   
   // NB: avoid copying the FTATMatchTimedEvent into the FTimerDelegate by binding a lambda to lookup by ID and pass to _AuthorityOnEventOccurred
   const FGameplayTag eventId = timedEvent.EventId;
   TWeakObjectPtr<UTATMatchTimedEventComponent> weakThis = MakeWeakObjectPtr(this);
   const FTimerDelegate timerDelegate = FTimerDelegate::CreateLambda([eventId, weakThis] 
      {
         if (weakThis.IsValid())
         {
            FTATMatchTimedEvent* timedEvent = weakThis->MatchedTimedEvents.FindByKey(eventId);
            check(timedEvent);

            constexpr float timeElapsedSinceOccurred = 0.f;
            weakThis->_AuthorityOnEventOccurred(*timedEvent, timeElapsedSinceOccurred); 
         }
      });
   constexpr bool looping = false;
   GetWorld()->GetTimerManager().SetTimer(timedEvent.AuthorityTimerHandle, timerDelegate, secondsUntilActivation, looping);
}

float UTATMatchTimedEventComponent::_GetSecondsUntilEvent(const FTATMatchTimedEvent& timedEvent) const
{
   const ATATGameState* tatGs = GetWorld()->GetGameState<ATATGameState>();
   check(tatGs);
   check(tatGs->HasTATMatchStarted());

   // FIXME: This whole way we specify timed events needs to be rethought
   //        with segregated phases. This is just a band-aid to make the current
   //        trivial use-cases (stashes opening immediately) work.
   //
   //        For now, it treats times as relative to the main phases, and
   //        assumes that jumping to the endgame would not skip the triggers.
   //        This is _not_ the final intended design.
   //        
   //        Per Design:
   //           "These days 'endgame' means an early end to the match. That
   //            includes the possibility of never seeing events that were scheduled
   //            to happen later, and that's ok."
   //
   //        But it is a more conservative band-aid in the short term, and too much
   //        stuff is hopefully more obvious than not enough stuff.
   if (tatGs->GetCurrentPhase() > ETATMatchPhase::Main)
   {
      return 0;
   }

   const float secondsUntilPhaseEnd = tatGs->GetTimeLeftInPhase();

   switch (timedEvent.EventType)
   {
   case ETATMatchTimedEventType::SecondsAfterMatchStart:
   {
      const float phaseDuration = tatGs->GetTimeLeftInPhase();
      const float secondsSinceMatchStart = phaseDuration - secondsUntilPhaseEnd;
      return timedEvent.Seconds - secondsSinceMatchStart;
   }
   case ETATMatchTimedEventType::SecondsBeforeMatchEnd:
   {
      return secondsUntilPhaseEnd - timedEvent.Seconds;
   }

   default:
      checkNoEntry();
   }

   return 0.f;
}

float UTATMatchTimedEventComponent::_GetDurationRemainingForEvent(const FTATMatchTimedEvent& timedEvent) const
{
   if (const FTATMatchTimedEventReplicatedState* replicatedEventState = _replicatedEventData.FindByKey(timedEvent))
   {
      const float timeElapsedSinceTriggered = UOSEInteractionHelpers::GetServerTimeForComparison(this) - replicatedEventState->ServerTimeEventTriggered;
      return FMath::Max(timedEvent.Duration - timeElapsedSinceTriggered, 0.f);
   }
   UE_LOG(LogTATMatchTimedEventComponent, Warning, TEXT("_GetDurationRemainingForEvent() called for %s which hasn't occurred yet!"), *timedEvent.ToString());
   return timedEvent.Duration;
}
