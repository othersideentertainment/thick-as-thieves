// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"
#include "Interactables/InteractableInterface.h"

// ue5
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"

#include "TATHidingSpot.generated.h"

class UGameplayEffect;
class UCapsuleComponent;

USTRUCT(BlueprintType)
struct FTATHidingSpotState
{
   GENERATED_BODY()

public:
   UPROPERTY(Transient)
   ACharacter* HidingCharacter = nullptr;

   UPROPERTY(Transient)
   float ChangedServerTime = 0;
};

UCLASS()
class TAT_API ATATHidingSpot : public AActor
   , public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
   ATATHidingSpot();

   // Interactable Interface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;

   UFUNCTION(BlueprintCallable)
   void TryFinishExit(ACharacter* exitingCharacter);

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

protected:
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UFUNCTION(BlueprintNativeEvent)
   AActor* GetIntendedViewTarget() const;
   virtual AActor* GetIntendedViewTarget_Implementation() { return this; }

   const class UCameraComponent* _FindFirstActiveCamera() const;
   void _SetHidingCharacter(ACharacter* hidingCharacter);

   void _OnPredictiveHideCatchup(TWeakObjectPtr<ACharacter> hidingCharacter);

   FVector _FindExitLocationForCharacter(const ACharacter* hidingCharacter) const;
   bool _ShouldFaceHidingSpotOnExit(const ACharacter* hidingCharacter) const;

   const ACharacter* _GetHidingCharacter() const { return _state.HidingCharacter; }

   // For FX hooks, will change structure once open/close
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnHidingCharacterRecentlyChanged(bool hasCharacter);

   UFUNCTION()
   void _OnRep_State(const FTATHidingSpotState& oldState);

private:
   // possibly replace with different effect later
   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _appliedEffects;

   UPROPERTY(EditDefaultsOnly, Category = Components)
   TObjectPtr<USceneComponent> _rootComponent;

   UPROPERTY(EditDefaultsOnly, Category = Components)
   TObjectPtr<UCapsuleComponent> _exitCapsuleComponent;

private:
   UPROPERTY(EditDefaultsOnly, Category = Hiding)
   TSubclassOf<UGameplayEffect> _hidingEffect;

   UPROPERTY(EditDefaultsOnly, Category = Hiding)
   FGameplayTag _hidingTag;

   UPROPERTY(EditDefaultsOnly, Category = Hiding)
   FText _hideInteractPrompt;

   UPROPERTY(EditDefaultsOnly, Category = Hiding)
   FText _hideFullMessage;

   UPROPERTY(EditDefaultsOnly, Category = Stims, meta = (Categories="AI.Stim.Hearing"))
   FGameplayTag _enterHearingStim;

   UPROPERTY(EditDefaultsOnly, Category = Stims, meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag _exitHearingStim;

   UPROPERTY(EditDefaultsOnly, Category = Transition)
   FViewTargetTransitionParams _enterCameraTransition;

   UPROPERTY(EditDefaultsOnly, Category = Transition)
   FViewTargetTransitionParams _exitCameraTransition;

   UPROPERTY(EditDefaultsOnly, Category = Requirements)
   FGameplayTagContainer _requiredEnterTags;

   UPROPERTY(EditDefaultsOnly, Category = Requirements)
   FGameplayTagContainer _blockedEnterTags;

private:
   UPROPERTY(ReplicatedUsing=_OnRep_State, Transient)
   FTATHidingSpotState _state;
};
