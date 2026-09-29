// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATExplicitTransitionToggle.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue5
#include "Components/TimelineComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATExplicitTransitionToggle)

// Sets default values for this component's properties
UTATExplicitTransitionToggleComponent::UTATExplicitTransitionToggleComponent()
{
   SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;

   _turnOnDuration = 10.f;
   _turnOffDuration = 10.f;
}


// Called when the game starts
void UTATExplicitTransitionToggleComponent::BeginPlay()
{
	Super::BeginPlay();

   _DispatchSyncTimelines();
   OnStateOrTransitionChanged.Broadcast(_state.IsOn, _state.IsTransitioning);
}

void UTATExplicitTransitionToggleComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UTATExplicitTransitionToggleComponent, _state);
}

void UTATExplicitTransitionToggleComponent::SyncTimelineWithExclusiveToggle(const FTATExplicitTransitionToggleState& state, UTimelineComponent* timeline, float durationOverride)
{
   check(timeline);
   const float kCloseEnoughThreshold = 0.5f;
   const float length = timeline->GetTimelineLength();
   const float stateAge = UOSEInteractionHelpers::GetServerTimeForComparison(timeline) - state.ChangedServerTime;

   if (state.IsTransitioning)
   {
      if (durationOverride > 0)
      {
         timeline->SetPlayRate(length / durationOverride);
      }

      if (state.IsOn)
      {
         timeline->Play();
      }
      else
      {
         timeline->Reverse();
      }

      if (stateAge > length)
      {
         const float unscaledTimeInState = FMath::Clamp(stateAge / timeline->GetPlayRate(), 0, length);
         const float unscaledTarget = state.IsOn ? unscaledTimeInState : (length - unscaledTimeInState);
         const float scaledDelta = (unscaledTarget - timeline->GetPlaybackPosition()) * timeline->GetPlayRate();

         if (FMath::Abs(scaledDelta) > kCloseEnoughThreshold)
         {
            // TODO: is it correct to fire events?
            timeline->SetPlaybackPosition(unscaledTarget, true);
         }
      }
   }
   else
   {
      float newPosition = state.IsOn ? length : 0;
      const float scaledDelta = (newPosition - timeline->GetPlaybackPosition()) * timeline->GetPlayRate();

      // if not transitioning, jump to the end if not pretty close
      if (!timeline->IsPlaying() || (timeline->IsReversing() == !state.IsOn) || FMath::Abs(scaledDelta) < kCloseEnoughThreshold)
      {
         timeline->SetPlaybackPosition(newPosition, true);
         timeline->Stop();
      }
   }
}

void UTATExplicitTransitionToggleComponent::AuthorityTransitionTo(bool newOn)
{
   check(GetOwner()->HasAuthority());

   if (newOn == IsOn()) return;

   GetOwner()->FlushNetDormancy();

   FTATExplicitTransitionToggleState oldState = _state;
   _state.IsOn = newOn;
   _state.IsTransitioning = true;
   _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   _OnRep_State(oldState);

   GetOwner()->GetWorldTimerManager().SetTimer(_transitionTimerHandle, this, &UTATExplicitTransitionToggleComponent::_OnTransitionComplete, _GetTransitionDuration(), false);
}

void UTATExplicitTransitionToggleComponent::_OnRep_State(const FTATExplicitTransitionToggleState& previousState)
{
   _DispatchSyncTimelines();

   if (previousState.IsOn != _state.IsOn || previousState.IsTransitioning != _state.IsTransitioning)
   {
      OnStateOrTransitionChanged.Broadcast(_state.IsOn, _state.IsTransitioning);
      if (!UOSEInteractionHelpers::IsOld(this, _state.ChangedServerTime, 0.75f))
      {
         OnStateOrTransitionChangedRecently.Broadcast(_state.IsOn, _state.IsTransitioning);
      }

      if (previousState.IsTransitioning != _state.IsTransitioning)
      {
         if (_state.IsTransitioning)
         {
            OnTransitionStarted.Broadcast(_state.IsOn);
         }
         else
         {
            OnTransitionEnded.Broadcast(_state.IsOn);
         }
      }
   }
}

void UTATExplicitTransitionToggleComponent::_DispatchSyncTimelines() const
{
   OnSyncTimelines.Broadcast(_state, _GetTransitionDuration());
}

float UTATExplicitTransitionToggleComponent::_GetTransitionDuration() const
{
   return _state.IsOn ? _turnOnDuration : _turnOffDuration;
}

void UTATExplicitTransitionToggleComponent::_OnTransitionComplete()
{
   if (!_state.IsTransitioning) return;

   GetOwner()->FlushNetDormancy();

   FTATExplicitTransitionToggleState oldState = _state;
   _state.IsTransitioning = false;
   _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   _OnRep_State(oldState);
}

