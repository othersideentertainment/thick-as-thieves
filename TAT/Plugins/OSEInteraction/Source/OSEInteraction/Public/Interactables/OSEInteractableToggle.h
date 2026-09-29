// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "InteractableInterface.h"
#include "OSEInteractionHelpers.h"
#include "OSEToggleInterface.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"

#include "OSEInteractableToggle.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSESyncedToggle, Warning, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToggleStateChanged, bool, isOn);

/// Used to control the allowed toggle-transitions for an AOSESyncedToggle
UENUM()
enum class EOSESyncedToggleAllowedTransition : uint8
{
   ToggleBothWays,
   OnlyToggleOn,
   OnlyToggleOff
};

// A simple base class for a toggle-able actor, with utilities to drive timeline animations from its state
UCLASS(Blueprintable)
class OSEINTERACTION_API AOSESyncedToggle : public AActor
   , public IOSEToggleInterface
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   AOSESyncedToggle();

protected:
   virtual void BeginPlay() override;
   virtual void GatherCurrentMovement() override;

public:
   UFUNCTION(BlueprintCallable, BlueprintPure)
   FORCEINLINE bool IsOn() const { return State.bIsOn; }

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void TurnOn() { SetOn(true); }

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void TurnOff() { SetOn(false); }

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   FORCEINLINE void Toggle() { SetOn(!IsOn()); }

   UFUNCTION(BlueprintPure)
   bool IsInPermanentToggledState() const;

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
   bool CanSwitchState() const;
   virtual bool CanSwitchState_Implementation() const { return !IsInPermanentToggledState(); }

   UPROPERTY(BlueprintAssignable)
   FOnToggleStateChanged OnToggleStateChanged;

   /// Controls the allowed toggle-transitions. Override if being toggled permanently to an on/off state is desirable.
   UPROPERTY(EditAnywhere, Category = Interactable)
   EOSESyncedToggleAllowedTransition AllowedToggleTransition = EOSESyncedToggleAllowedTransition::ToggleBothWays;
   
   virtual void SetOn(bool newOn);

   // IOSEToggleInterface
   virtual bool IsToggleOn() const override final { return IsOn(); }
   virtual void SetToggleOn(bool isOn) override final { SetOn(isOn); }
   // IOSEToggleInterface end
protected:
   UFUNCTION(BlueprintImplementableEvent)
   void SyncTimelines(const FOSEToggleState& NewState);

   // Called when the state is changed (including at start). May not be recent
   UFUNCTION(BlueprintImplementableEvent)
   void OnStateChanged(bool bIsOn, bool bWasRecent);

   // Called when State.bIsOn has recently changed. Use for ephemeral effects, like sounds
   UFUNCTION(BlueprintImplementableEvent)
   void OnRecentlyToggled(bool bIsOn);

   virtual void _OnStateChanged(bool bIsOn, bool bWasRecent);


protected:
   UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_State, Category = Interactable)
   FOSEToggleState State;

   UFUNCTION()
   void OnRep_State(const FOSEToggleState& PreviousState);
};

// A simple base class for a toggle-able interactable, with utilities to drive timeline animations from its state
UCLASS(Blueprintable)
class OSEINTERACTION_API AOSEInteractableToggle : public AOSESyncedToggle, public IInteractableInterface
{
   GENERATED_BODY()

public:
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* InteractingCharacter) override;

protected:
   virtual void ToggleForInteraction(ACharacter* interactingCharacter);

   UFUNCTION(BlueprintNativeEvent)
   void OnAuthorityToggledBy(ACharacter* interactingCharacter, bool isOn);
   virtual void OnAuthorityToggledBy_Implementation(ACharacter* interactingCharacter, bool isOn) {}

protected:
   UPROPERTY(EditAnywhere, Category = Interactable, AdvancedDisplay)
   FText TurnOnPrompt;

   UPROPERTY(EditAnywhere, Category = Interactable, AdvancedDisplay)
   FText TurnOffPrompt;

   UPROPERTY(EditAnywhere, Category = Interactable, AdvancedDisplay)
   FGameplayTag InteractionStatusTag;

   UPROPERTY(EditDefaultsOnly, Category = Interactable, AdvancedDisplay, meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag TurnOnAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = Interactable, AdvancedDisplay, meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag TurnOffAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = Interactable, AdvancedDisplay)
   FGameplayTag StatTagOff;

   UPROPERTY(EditDefaultsOnly, Category = Interactable, AdvancedDisplay)
   FGameplayTag StatTagOn;
};
