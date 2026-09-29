// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "GameFramework/MatchTimedEvents/TATMatchTimedEventTypes.h"

#include "TATMatchTimedEventComponent.generated.h"

enum class ETATMatchPhase : uint8;

// Replicated data for a client to track a triggered event's duration
USTRUCT()
struct TAT_API FTATMatchTimedEventReplicatedState
{
   GENERATED_BODY()

   FTATMatchTimedEventReplicatedState() {}
   FTATMatchTimedEventReplicatedState(const FTATMatchTimedEvent& matchTimedEvent, float timeTriggered) 
      : EventId(matchTimedEvent.EventId), ServerTimeEventTriggered(timeTriggered) {}

public:
   UPROPERTY()
   FGameplayTag EventId = FGameplayTag::EmptyTag;

   UPROPERTY()
   float ServerTimeEventTriggered = 0;

   FORCEINLINE bool operator==(const FTATMatchTimedEvent& matchTimedEvent) const { return EventId == matchTimedEvent.EventId; }
};

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/// Component for defining gameplay events that should occur at specific times in the match.
/// 
/// Events can optionally have a duration for which they last.
/// 
/// Event timers are managed solely by the server. While clients can query the remaining time until an event occurs/elapses, listeners are responsible for replicating 
/// state changes down to clients.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
UCLASS(meta = (BlueprintSpawnableComponent), HideCategories=(Collision, ComponentReplication, Components, ComponentTick, Cooking, LOD, Object, Physics, Rendering, Utilities, AssetUserData, Tags))
class TAT_API UTATMatchTimedEventComponent : public UActorComponent
{
   GENERATED_BODY()

   UTATMatchTimedEventComponent();

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR
   virtual void PostLoad() override;

   // From UActorComponent
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void Activate(bool reset) override;
   virtual void Deactivate() override;
   
public:

   UFUNCTION(BlueprintPure, Category = "TAT|Match Timed Event")
   FORCEINLINE int GetNumTimers() const { return MatchedTimedEvents.Num(); }
   UFUNCTION(BlueprintPure, Category = "TAT|Match Timed Event")
   FORCEINLINE bool HasAnyTimers() const { return MatchedTimedEvents.Num() > 0; }

   /// Returns time in seconds until event will occur or, if it has already, the time elapsed since then (as a negative value)
   UFUNCTION(BlueprintPure, Category = "TAT|Match Timed Event")
   float GetTimeSecondsUntilEvent(FGameplayTag eventId, bool& found) const;

   FORCEINLINE float GetTimeSecondsUntilEvent(FGameplayTag eventId) const
   {
      bool found = false;
      return GetTimeSecondsUntilEvent(eventId, found);
   }

   /// Returns duration remaining in triggered event, or 0 if elapsed
   UFUNCTION(BlueprintPure, Category = "TAT|Match Timed Event")
   float GetDurationSecondsRemainingForEvent(FGameplayTag eventId, bool& found) const;

   FORCEINLINE float GetDurationSecondsRemainingForEvent(FGameplayTag eventId) const
   {
      bool found = false;
      return GetDurationSecondsRemainingForEvent(eventId, found);
   }

   FORCEINLINE const FTATMatchTimedEvent* GetMatchTimedEvent(FGameplayTag eventId) const
   {
      return MatchedTimedEvents.FindByKey(eventId);
   }

   void SetDurationSupportedByDefault(bool durationSupported);

private:
   void _AuthorityOnMatchStart();

   UFUNCTION()
   void _AuthorityOnPhaseTimeChanged(ETATMatchPhase phase);

   void _AuthorityOnEventOccurred(FTATMatchTimedEvent& matchTimedEvent, float timeElapsedSinceOccurred);

   /// Sets timer for event start time or, if elapsed, its remaining duration.
   void _AuthoritySetTimerForEvent(FTATMatchTimedEvent& matchTimedEvent, float secondsUntilActivation);

   /// Returns time in seconds until event will occur or, if it has already, the time elapsed since then (as a negative value)
   float _GetSecondsUntilEvent(const FTATMatchTimedEvent& matchTimedEvent) const;

   /// Returns duration remaining in triggered event, or 0 if elapsed
   float _GetDurationRemainingForEvent(const FTATMatchTimedEvent& matchTimedEvent) const;

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimedEventOccurred, FGameplayTag, eventId, float, timeElapsedSinceOccurred);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimedEventDurationElapsed, FGameplayTag, eventId);

   UPROPERTY(BlueprintAssignable)
   FOnTimedEventOccurred AuthorityOnTimedEventOccurred;
   
   UPROPERTY(BlueprintAssignable)
   FOnTimedEventDurationElapsed AuthorityOnTimedEventDurationElapsed;

   /// Collection of events that should occur at specific times in the match
   UPROPERTY(EditAnywhere, Category = "Match Timers")
   TArray<FTATMatchTimedEvent> MatchedTimedEvents;

private:
   /// Replicated collection of each event and its time of activation
   UPROPERTY(Transient, Replicated)
   TArray<FTATMatchTimedEventReplicatedState> _replicatedEventData;

   /// Allows owning actor to hide the duration field if its usage would clash with the use case
   bool _durationSupported = true;
};
