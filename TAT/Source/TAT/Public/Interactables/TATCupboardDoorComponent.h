// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "Components/StaticMeshComponent.h"
#include "Internationalization/Text.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "Interactables/OSEInteractionHelpers.h"
#include "Lockpicking/LockpickableInterface.h"

// tat
#include "TATLockableInterface.h"
#include "TATLockConfig.h"

#include "TATCupboardDoorComponent.generated.h"

class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSyncTimelines, const FOSEToggleState&, state);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToggled, bool, bIsOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecentlyToggled, bool, bIsOpen);

// This is a component class intended for simple interactable components on existing actors
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TAT_API UTATCupboardDoorComponent : public UStaticMeshComponent,
                                          public IInteractableInterface,
                                          public ILockpickableInterface,
                                          public ITATLockableInterface

{
   GENERATED_BODY()

public:
   UTATCupboardDoorComponent();

   // begin AActorComponent
   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   // end AActorComponent
   
   // begin IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;
   // end IInteractableInterface
   
   UPROPERTY(BlueprintAssignable, Category = "TAT|Interactable")
   FSyncTimelines SyncTimelines;

   // For persistent or looping feedback
   UPROPERTY(BlueprintAssignable, Category="TAT|Interactable")
   FOnToggled OnDoorToggled;

   // Fire one-offs associated with the act of toggling
   UPROPERTY(BlueprintAssignable, Category = "TAT|Interactable")
   FOnRecentlyToggled OnDoorRecentlyToggled;

   UFUNCTION(BlueprintPure, Category="TAT|Interactable")
   bool IsOpen() const
   {
      return _state.bIsOn;
   }

   // begin ITATLockableInterface
   virtual FLockInteractContext MakeLockContext(ACharacter* interactingCharacter) const override;
   // end ITATLockableInterface
   
   UFUNCTION(BlueprintCallable, BlueprintPure)
   FORCEINLINE bool IsLocked() const { return _locked; }
   
   // begin ILockpickableInterface
   virtual void Unlock() override { SetLocked(false); }
   virtual void Lock() override { SetLocked(true); }
   virtual void LockWithKey() override { SetLocked(true); }
   virtual void OnLockpickTrackCompleted(int32 trackIndex) override;
   virtual int32 GetLockpickCurrentTrack() override { return _lockpickCurrentTrack; }
   virtual FTATOnRequestCancelLockpicking* GetOnRequestCancelLockpickingDelegate() override { return &_onRequestCancelLockpicking; }
   // end ILockpickableInterface
   
   void SetLocked(bool newIsLocked);

protected:

   UFUNCTION()
   void _OnRep_State(const FOSEToggleState& previousState);

   // Player facing prompt for opening the door
   UPROPERTY(EditAnywhere, Category="TAT|Interactable")
   FText _openPrompt;

   // Player facing prompt for closing the door
   UPROPERTY(EditAnywhere, Category="TAT|Interactable", meta = (DisplayAfter="_openPrompt"))
   FText _closePrompt;

   UPROPERTY(EditAnywhere, ReplicatedUsing = _OnRep_State, Category="TAT|Interactable")
   FOSEToggleState _state;
   
   UPROPERTY(EditAnywhere, Category="TAT|Lock")
   bool _canEverLock { true };
   
   UPROPERTY(Replicated, EditAnywhere, Category = "TAT|Lock", meta=(EditCondition="_canEverLock"))
   bool _locked { false };
   
   UPROPERTY(Replicated, Transient)
   int32 _lockpickCurrentTrack = 0;
   
   UPROPERTY(EditAnywhere, Category="TAT|Lock")
   FTATLockConfig _lockConfig;
   
   FTATOnRequestCancelLockpicking _onRequestCancelLockpicking;

private:
   void _Toggle();
};
