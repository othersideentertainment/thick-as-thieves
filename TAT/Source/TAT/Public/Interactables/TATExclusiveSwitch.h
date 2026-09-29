// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Interactables/TATInteractableToggle.h"

#include "TATExclusiveSwitch.generated.h"


class ATATExclusiveSwitchSet;
class UTATExplicitTransitionToggleComponent;

///
/// A switch that controls a toggleable in a set of mutually exclusive switches
/// 
/// Only one in the same set can be the not-default state
/// It cannot be used during a transition
/// 
/// Note: Not meaningfully client-predicted, unlike most interactable toggles
UCLASS()
class TAT_API ATATExclusiveSwitch : public ATATInteractableToggle
{
	GENERATED_BODY()

public:
   ATATExclusiveSwitch();

protected:
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintPure)
   bool IsTargetTransitioning() const;

   UFUNCTION(BlueprintPure)
   bool IsOtherSwitchInUse() const;

   UFUNCTION(BlueprintPure)
   UTATExplicitTransitionToggleComponent* GetTargetToggleable() const;
 
   // interactable interface
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;

protected:
   virtual void ToggleForInteraction(ACharacter* interactingCharacter) override;

   bool _CanCurrentlyToggle() const;

#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif

   void _UpdateOtherSwitchInUse();
   UFUNCTION()
   void _OnSetInUseChanged(bool inUse);

   void _MaybeSchedulePredictionRollbackCheck();
   void _CheckForPredictionRollback();

   bool _IsTargetProbablyTransitioning() const;

   UFUNCTION()
   void _OnTargetStateOrTransitionChanged(bool isOn, bool isTransitioning);

   UFUNCTION()
   void _OnTargetStateOrTransitionChangedRecently(bool isOn, bool isTransitioning);

   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName= OnTargetStateOrTransitionChanged))
   void BP_OnTargetStateOrTransitionChanged(bool isOn, bool isTransitioning);

   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = OnTargetStateOrTransitionChangedRecently))
   void BP_OnTargetStateOrTransitionChangedRecently(bool isOn, bool isTransitioning);

   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = OnOtherSwitchInUseChanged))
   void BP_OnOtherSwitchInUseChanged(bool isOtherSwitchInUse);

   // *not replicated*
   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = OnFailedToUseLocally))
   void BP_OnFailedToUseLocally(ACharacter* interactingCharacter);

protected:
   UPROPERTY(EditInstanceOnly, Category = "Target", meta = (UseComponentPicker, AllowAnyActor, AllowedClasses = "/Script/TAT.TATExplicitTransitionToggleComponent"))
   FSoftComponentReference _targetToggleable;

   UPROPERTY(Transient)
   TObjectPtr<UTATExplicitTransitionToggleComponent> _cachedToggleable;

   UPROPERTY(EditInstanceOnly, Category = "Target")
   TObjectPtr<ATATExclusiveSwitchSet> _switchSet;

   UPROPERTY(EditAnywhere, Category = Interactable, AdvancedDisplay)
   FText AlreadyTransitioningErrorPrompt;

   UPROPERTY(EditAnywhere, Category = Interactable, AdvancedDisplay)
   FText InUseErrorPrompt;

   UPROPERTY(EditAnywhere, Category = Interactable, AdvancedDisplay)
   FText InUseErrorMessage;

   // the delay before checking to rollback a predictively change to a switch
   // Will cause stuttering if not higher than round-trip latency
   UPROPERTY(EditDefaultsOnly, Category = "Interactable", AdvancedDisplay)
   float _predictionCheckRollbackWindow;

   FTimerHandle _checkRollbackTimerHandle;
   float _checkRollbackTimestamp;

   bool _isOtherSwitchInUse;
};
