// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TATExplicitTransitionToggle.generated.h"

class UTimelineComponent;

USTRUCT(BlueprintType)
struct FTATExplicitTransitionToggleState
{
   GENERATED_BODY()

public:
   // Server time of the last time that the toggle was changed
   UPROPERTY(Transient)
   float ChangedServerTime = 0;

   UPROPERTY(EditAnywhere)
   bool IsOn = false;

   UPROPERTY(Transient)
   bool IsTransitioning = false;
};

// A component that can be toggles between two states with a timed transition that is explicitly replicated
//
// Initially for use by ATATExclusiveSwitch
// 
// It *not* intended to be a replacement for interactable toggles, but specifically things that need
// an explicit transition state with gameplay effects, as it has side effects and trade-offs that come
// with that.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UTATExplicitTransitionToggleComponent : public UActorComponent
{
	GENERATED_BODY()

public:

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSyncTimelines, FTATExplicitTransitionToggleState, state, float, transitionDuration);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, bool, isOn, bool, isTransitioning);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransitionChanged, bool, isOn);

public:	
	// Sets default values for this component's properties
	UTATExplicitTransitionToggleComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
   UFUNCTION(BlueprintCallable, Category = "Interactable|Toggle|ExclusiveTransition")
   static void SyncTimelineWithExclusiveToggle(const FTATExplicitTransitionToggleState& state, UTimelineComponent* timeline, float durationOverride);

   UFUNCTION(BlueprintPure, Category = "Interactable|Toggle|ExclusiveTransition")
   bool IsOn() const { return _state.IsOn; }

   UFUNCTION(BlueprintPure, Category = "Interactable|Toggle|ExclusiveTransition")
   bool IsTransitioning() const { return _state.IsTransitioning; }

   UFUNCTION(BlueprintPure, Category = "Interactable|Toggle|ExclusiveTransition")
   bool IsFullyInState(bool isOn) const { return !_state.IsTransitioning && isOn == _state.IsOn; }

   UFUNCTION(BlueprintPure, Category = "Interactable|Toggle|ExclusiveTransition")
   bool IsTransitioningTo(bool isOn) const { return _state.IsTransitioning && isOn == _state.IsOn; }

   UFUNCTION(BlueprintAuthorityOnly , Category = "Interactable|Toggle|ExclusiveTransition")
   void AuthorityTransitionTo(bool newOn);

public:
   UPROPERTY(BlueprintAssignable)
   FOnSyncTimelines OnSyncTimelines;

   // Called when the state (or transitioning-ness) is changed (including at start). May not be recent
   UPROPERTY(BlueprintAssignable)
   FOnStateChanged OnStateOrTransitionChanged;

   // Called when the state (or transitioning-ness) is changed recently. Use this for ephemeral effects
   UPROPERTY(BlueprintAssignable)
   FOnStateChanged OnStateOrTransitionChangedRecently;

   // Called when the state has changed from a non-transition to a transition (may or may not be recent)
   UPROPERTY(BlueprintAssignable)
   FOnTransitionChanged OnTransitionStarted;

   // Called when the state has changed from a transition to a non-transition (may or may not be recent)
   UPROPERTY(BlueprintAssignable)
   FOnTransitionChanged OnTransitionEnded;

protected:
   UPROPERTY(EditAnywhere, ReplicatedUsing = _OnRep_State, Category = Toggle)
   FTATExplicitTransitionToggleState _state;
   FTimerHandle _transitionTimerHandle;

   UPROPERTY(EditDefaultsOnly, Category = Toggle)
   float _turnOnDuration;

   UPROPERTY(EditDefaultsOnly, Category = Toggle)
   float _turnOffDuration;

   UFUNCTION()
   void _OnRep_State(const FTATExplicitTransitionToggleState& previousState);
   void _DispatchSyncTimelines() const;
   float _GetTransitionDuration() const;

   UFUNCTION()
   void _OnTransitionComplete();
};
