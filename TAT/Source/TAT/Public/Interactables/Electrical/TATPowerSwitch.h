// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "Components/ActorComponent.h"

#include "TATPowerSwitch.generated.h"

class ATATPowerSource;

/// An interactable switch used to toggle the state of a TATPowerSource (and naturally the devices dependent on said power source).
/// TIP: A shared set of TATElectricalDevices can be controlled by multiple switches referencing the same power source.
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class TAT_API UTATPowerSwitch : public UActorComponent, public IInteractableInterface
{
   GENERATED_BODY()
   
#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // From IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

public:
   UFUNCTION(BlueprintPure, Category = "TAT|Electrical")
   ATATPowerSource* GetPowerSource() const;

   // Should be called by execution of _toggleGameplayCue
   UFUNCTION(BlueprintCallable)
   void BroadcastUsed() { OnUsed.Broadcast(); }

protected:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToggled);
   // Called on the server + interacting client when used by a character. Use for cosmetic one-shot behavior
   UPROPERTY(BlueprintAssignable, Category = "TAT|Interactable")
   FOnToggled OnUsed;

private:
   UPROPERTY(EditInstanceOnly, Category = "TAT|Electrical")
   TObjectPtr<ATATPowerSource> _powerSource;

   UPROPERTY(EditAnywhere, Category="TAT|Interaction")
   bool _usePowerSourcePrompts = false;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Interaction", meta = (EditCondition = "!_usePowerSourcePrompts"))
   FText _turnOnPrompt;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Interaction", meta = (EditCondition = "!_usePowerSourcePrompts"))
   FText _turnOffPrompt;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Interaction", meta = (Categories="InteractAnimation.Instant"))
   FGameplayTag _interactAnimationTag;

   // Executed gameplay cue responsible for performing one-off toggle feedback (eg. animation, SFX)
   UPROPERTY(EditDefaultsOnly, Category="TAT|Interaction", meta = (Categories="GameplayCue"))
   FGameplayTag _switchUsedGameplayCue;
};
