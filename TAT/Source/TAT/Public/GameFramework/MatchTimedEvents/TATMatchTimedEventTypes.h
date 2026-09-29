// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "GameplayTagContainer.h"

#include "TATMatchTimedEventTypes.generated.h"

UENUM(BlueprintType)
enum class ETATMatchTimedEventType : uint8
{
   // Event should fire X seconds after match start
   // TODO: Needs to be reworked for match phases
   SecondsAfterMatchStart UMETA(DisplayName = "Seconds After Main Phase Start (WIP)"),
   // Event should fire X seconds before main phase end
   // TODO: Needs to be reworked for match phases
   SecondsBeforeMatchEnd UMETA(DisplayName = "Seconds Before Main Phase End (WIP)")     
};

enum class ETATMatchTimedEventState : uint8
{
   PendingEvent,        // Event hasn't occurred yet
   PendingDuration,     // Event has occurred, and duration has yet to elapse
   Elapsed              // Event and duration (if any) has elapsed
};

/// Defines a time at which an event should take place (see UTATMatchTimedEventComponent)
USTRUCT()
struct TAT_API FTATMatchTimedEvent
{
   GENERATED_BODY()

public:
   FORCEINLINE bool HasDuration() const { return ExposeDuration && Duration > 0; }

   FORCEINLINE FString ToString() const { return FString::Printf(TEXT("FTATMatchTimedEvent(%s)"), *EventId.ToString()); }
   FORCEINLINE bool operator==(const FGameplayTag& eventId) const { return EventId == eventId; }
   FORCEINLINE bool operator==(const FTATMatchTimedEvent& timedEvent) const { return EventId == timedEvent.EventId; }
   
public:
   // Defines whether the event timing is relative to match start/end
   UPROPERTY(EditAnywhere, Category = "Timing")
   ETATMatchTimedEventType EventType = ETATMatchTimedEventType::SecondsAfterMatchStart;

   // Timing of the event (relative to start/end of match, according to EventType)
   UPROPERTY(EditAnywhere, Category = "Timing", meta = (UIMin = "0.1", ClampMin = "0.1", Units = "seconds"))
   float Seconds = 0.1f;

   // Optional duration for the event
   UPROPERTY(EditAnywhere, Category = "Timing", meta = (UIMin = "0", ClampMin = "0", Units = "seconds", EditCondition = "ExposeDuration", EditConditionHides))
   float Duration = 0;

   // Allows component owner to control whether events should expose an assignable duration.
   // Some use cases may warrant an actor managing an event's duration itself (such as when it should begin after the event + some arbitrary event has occurred),
   // so hiding this event's duration field may be preferable to avoid designer confusion.
   UPROPERTY()
   bool ExposeDuration = true;

   // Unique identifier for event
   UPROPERTY(EditAnywhere, meta = (Categories = "MatchTimedEvent"))
   FGameplayTag EventId;

   // Tracks the state of this event pertaining to its associated timer
   ETATMatchTimedEventState AuthorityEventState = ETATMatchTimedEventState::PendingEvent;
   FTimerHandle AuthorityTimerHandle;
};
