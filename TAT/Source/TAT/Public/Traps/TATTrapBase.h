// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Environment/TATInhibitableInterface.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue5
#include "GameFramework/Actor.h"
#include "ScalableFloat.h"

#include "TATTrapBase.generated.h"


class UTATTrapDetectorComponent;
class UTATElectricalDeviceComponent;

// Possible states for simple traps
UENUM(BlueprintType)
enum class ETATTrapState : uint8
{
   Ready,
   // The trap is currently triggering
   Triggering,
   // The trap has been triggered, and is not expected to reset
   // TODO: this state should probably be renamed to be clearer now that resetting is a thing (and this can go to that). The BP_AuthorityTriggerComplete should _not_ be renamed with that
   TriggerComplete,
   // The trap is resetting, and will got back to ready
   Resetting,
   // The trap has been disarmed
   Disarmed
   // CONSIDER: Represent unpowered as a state?
};

USTRUCT()
struct FTATTrapState
{
   GENERATED_BODY()

   // Server time of the last time that the toggle was changed
   UPROPERTY(Transient)
   float ChangedServerTime = 0;

   UPROPERTY(EditAnywhere)
   ETATTrapState State = ETATTrapState::Ready;
};

// A base-class for simple traps
//
// NOTE: This is a base class for some implementations, but should not be used
//       as the identity of a trap.
UCLASS(Abstract)
class TAT_API ATATTrapBase
   : public AActor
   , public IInteractableInterface
   , public ITATInhibitableInterface
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ATATTrapBase();

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityTrigger(AActor* optionalTarget);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   virtual void AuthorityDisarm();

   // Re-arms the trap if it is disarmed or fully triggered
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   virtual void AuthorityRearm();
public:
   UFUNCTION(BlueprintCallable, BlueprintPure)
   ETATTrapState GetState() const { return _state.State; }

   bool IsInState(ETATTrapState state) const { return GetState() == state; }
   bool IsArmed() const;

   // InteractableInterface start
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;
   // InteractableInterface end

   // TATInhibitableInterface start
   virtual bool CanBeInhibitedBy_Implementation(FGameplayTag inhibitorType) const override { return IsInhibitable; }
   virtual FGameplayTag GetInhibitableType_Implementation() const override { return InhibitableType; }
   virtual FTATInhibitorPlacementInfo GetInhibitorPlacementInfo_Implementation() const override;
   virtual void OnInhibitorActivated_Implementation(ATATInhibitorActor* inhibitorActor, APawn* instigator, int32 newInhibitorCount) override {}
   virtual void OnInhibitorDeactivated_Implementation(ATATInhibitorActor* inhibitorActor, int32 newInhibitorCount, bool allInhibitorsRemoved) override {}
   // TATInhibitableInterface end

protected:
   /// Allow inhibitors to work on this trap
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable")
   bool IsInhibitable = false;

   /// The type/category of this trap in terms of things that can be inhibited in the world.
   /// Allows things that apply inhibitors to decide what they can inhibit (eg. a tool that can inhibit this actor only if it has the type "Inhibitable.Trap")
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable", Meta = (EditCondition = "IsInhibitable", Categories = "Inhibitable"))
   FGameplayTag InhibitableType;

   UFUNCTION()
   void _OnRep_State(const FTATTrapState& previousState);

   void _OnStateChanged(const FTATTrapState& previousState);
   virtual void _HandleStateChanged(const FTATTrapState& previousState) {}

   /// The trap has triggered on authority.
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnAuthorityTriggeredBy", meta = (BlueprintProtected, ScriptName = "OnAuthorityTriggeredBy"))
   void BP_OnAuthorityTriggeredBy(AActor* optionalTarget);

   /// The trap has completed its triggering phase on authority
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnAuthorityTriggerComplete", meta = (BlueprintProtected, ScriptName = "OnAuthorityTriggerComplete"))
   void BP_OnAuthorityTriggerComplete();

   /// The state has changed, but it may have changed a long time ago
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnStateChanged", meta = (BlueprintProtected, ScriptName = "OnStateChanged"))
   void BP_OnStateChanged(ETATTrapState newState, ETATTrapState previousState, bool wasRecent);

   /// The state has changed recently. This is a good place to play ephemeral things like SFX
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnStateChangedRecently", meta = (BlueprintProtected, ScriptName = "OnStateChangedRecently"))
   void BP_OnStateChangedRecently(ETATTrapState newState, ETATTrapState previousState);

   virtual bool _IsTriggeringAllowed() const;
   virtual void _OnAuthorityTriggeredBy(AActor* optionalTarget);
   void _SetState(ETATTrapState _newState);
   void _AuthoritySetTriggerComplete();
   void _OnTriggeringTimerComplete();
   void _OnResettingTimerComplete();

private:
   UPROPERTY(ReplicatedUsing = _OnRep_State, Transient)
   FTATTrapState _state;

   // How long (in seconds) the triggering state lasts (skips completely if 0)
   UPROPERTY(EditAnywhere, Category = Trap)
   float _triggeringDuration;

   // How long (in seconds) the trap resets in (does not reset if 0)
   UPROPERTY(EditAnywhere, Category = Trap)
   float _resetDuration;

   UPROPERTY(EditDefaultsOnly, Category = "Trap|Stims", meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag _triggerHearingStim;

   // Location in actor-location space where the hearing stim should be emitted
   UPROPERTY(EditDefaultsOnly, Category = "Trap|Stims")
   FVector _hearingStimOffset;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess="True"))
   TObjectPtr<UTATTrapDetectorComponent> _trapDetector;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
   TObjectPtr<UTATElectricalDeviceComponent> _electricalDeviceComponent;

   UPROPERTY(EditDefaultsOnly, Category="Electricity")
   bool _requirePowerToTrigger = false;

   FTimerHandle _nextStateTimerHandle;
};
