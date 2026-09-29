// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"

// tat
#include "Interactables/TATLockConfig.h"
#include "Lockpicking/LockpickableInterface.h"
#include "Traps/Old/TrapTriggerInterface.h"

// ue5
#include "GameFramework/Actor.h"

#include "TrapTriggerBase.generated.h"

class ITrapEmitterInterface_Old;

// Possible states for simple trap triggers
UENUM(BlueprintType, meta = (Scriptname = "TrapTriggerStateType"))
enum class ETrapTriggerState : uint8
{
   Ready,
   // Pressure Plate only: Something is on the plate, and will be triggered once they leave
   Depressed,
   // The trap has been triggered, and is not expected to reset
   Triggered,
   // The trap is resetting ofter being triggered
   Resetting,
   // The trap has been disarms
   Disarmed
};

/** Notification delegate definition for when the trap trigger state changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTATTrapTriggerStateChanged, ETrapTriggerState, newState, ETrapTriggerState, previousState, bool, isRecent);

USTRUCT(BlueprintType)
struct FTrapTriggerState
{
   GENERATED_BODY()

   // Server time of the last time that the toggle was changed
   UPROPERTY(Transient)
   float ChangedServerTime = 0;

   UPROPERTY(EditAnywhere)
   ETrapTriggerState State = ETrapTriggerState::Ready;
};

// A base-class for simple trap triggers
//
// NOTE: This is a base class for some implementations, but should not be used
//       as the identity of a trigger. That should be the domain of future
//       interfaces (which this would implement). -> See TrapTriggerInterface.
UCLASS(Abstract)
class TAT_API ATrapTriggerBase_Old : public AActor, public IInteractableInterface, public ILockpickableInterface, public ITrapTriggerInterface_Old
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ATrapTriggerBase_Old();

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityTrigger();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   virtual void AuthorityDisarm();

public:
   UFUNCTION(BlueprintCallable, BlueprintPure)
   ETrapTriggerState GetState() const { return _state.State; }

   bool IsInState(ETrapTriggerState state) const { return GetState() == state; }
   bool IsArmed() const;
   bool IsCurrentlyArmed() const;
   bool IsIndefinitelyDisarmed() const;
   void AuthorityDisableTemporarily(float disabledDuration);

   // InteractableInterface start
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;
   // InteractableInterface end

   // LockpickableInterface
   virtual void Unlock() override;
   virtual bool CanLockpickFail() const override { return true; }
   virtual void OnLockpickFailed() override;
   virtual void OnLockpickTrackCompleted(int32 trackIndex) override;
   virtual int32 GetLockpickCurrentTrack() override { return _lockpickCurrentTrack; }

   // TrapTriggerInterface
   virtual bool IsArmed_Implementation() const override;
   virtual ETrapTriggerType GetTriggerType_Implementation() const override { return triggerType; }

   UPROPERTY(BlueprintAssignable, Category = "TAT|Traps")
   FTATTrapTriggerStateChanged OnStateChanged;

protected:
   UPROPERTY(EditAnywhere)
   TArray<TScriptInterface<ITrapEmitterInterface_Old>> Emitters;

   UPROPERTY(EditAnywhere)
   ETrapTriggerType triggerType;

protected:
   UFUNCTION()
   void OnRep_State(const FTrapTriggerState& previousState);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientOnTriggered();

   /// Calculates the desired delay after the trap is triggered before emitters are fired
   /// This gives some flexibility to the implementation to derive this value without overly constraining the ability to optimize on dedicated servers
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "GetDelayBeforeFiringEmitters", meta = (BlueprintProtected, ScriptName = "GetDelayBeforeFiringEmitters"))
   float GetDelayBeforeFiringEmitters() const;

   /// The trap has triggered. Please only use for ephemeral stuff on non-authority
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnTriggered", meta = (BlueprintProtected, ScriptName = "OnTriggered"))
   void K2_OnTriggered();

   /// The state has changed, but it may have changed a long time ago
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnStateChanged", meta = (BlueprintProtected, ScriptName = "OnStateChanged"))
   void K2_OnStateChanged(ETrapTriggerState newState, ETrapTriggerState previousState, bool wasRecent);

   /// The state has changed recently. This is a good place to play ephemeral things like SFX
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnStateChangedRecently", meta = (BlueprintProtected, ScriptName = "OnStateChangedRecently"))
   void K2_OnStateChangedRecently(ETrapTriggerState newState, ETrapTriggerState previousState);

   virtual ETrapTriggerState GetStateAfterReset() const;

   void _SetState(ETrapTriggerState _newState);

private:
   UFUNCTION()
   void _OnResetTimer();

   void _TriggerEmitters();

private:
   UPROPERTY(ReplicatedUsing = OnRep_State, Transient)
   FTrapTriggerState _state;

   UPROPERTY(EditAnywhere, Category = Config)
   bool _canDisarmMechanically;

   UPROPERTY(EditAnywhere, Category = Config)
   bool _resetAfterTriggered;

   UPROPERTY(EditAnywhere, Category = Config)
   float _resetDuration;

   UPROPERTY(Replicated, Transient)
   int32 _lockpickCurrentTrack = 0;;

   UPROPERTY(EditAnywhere, Category="Disarming")
   FTATLockConfig _lockConfig;

   FTimerHandle _resetTimerHandle;
};
