// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Breakables/TATBreakableBase.h"
#include "AI/SmartObjects/TATAIIncorrectObjectStateInterface.h"
#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "Interactables/OSEInteractionHelpers.h"
#include "Interactables/OSEToggleInterface.h"
#include "Perception/AISightTargetInterface.h"

// wwise
#include "AkAudioEvent.h"

#include "TATPowerSource.generated.h"

class UTATEndgameActionComponent;
class UTATSecurityLockdownComponent;
DECLARE_LOG_CATEGORY_EXTERN(LogTATPowerSource, Warning, All);

class UStaticMeshComponent;

UCLASS(Blueprintable)
class TAT_API ATATPowerSource
   : public ATATBreakableBase
   , public IInteractableInterface
   , public ITATAIIncorrectObjectStateInterface
   , public ITATSmartObjectTagInterface
   , public ITATSmartObjectOwnerInterface
   , public IAISightTargetInterface
   , public IOSEToggleInterface
{
   GENERATED_BODY()

public:
   ATATPowerSource();

   // IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

   // IOSEToggleInterface
   virtual bool IsToggleOn() const override final { return IsSwitchedOn(); }
   virtual void SetToggleOn(bool isOn) override final { if(HasAuthority()) { AuthoritySetSwitchedOn(isOn); } }
   
   // IAISightTargetInterface start
   virtual bool CanBeSeenFrom(const FVector& observerLocation,
                              FVector& outSeenLocation,
                              int32& numberOfLoSChecksPerformed,
                              float& outSightStrength,
                              const AActor* ignoreActor = nullptr,
                              const bool* wasVisible = nullptr,
                              int32* userData = nullptr) const override;
   // IAISightTargetInterface end

   // Use actAsIfAlwaysSet is setting the initial value (prevents extra callbacks)
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TAT|Electrical")
   void AuthoritySetSwitchedOn(bool switchedOn, bool actAsIfAlwaysSet = false);

   UFUNCTION(BlueprintPure, Category = "TAT|Electrical")
   bool IsSwitchedOn() const
   {
      return _state.bIsOn;
   }

   UFUNCTION(BlueprintPure, Category = "TAT|Electrical")
   bool IsPowered() const;

   // from ITATSmartObjectOwnerInterface
   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override { return _incorrectStateActionNodeComponent; }
   
   // from ITATAIIncorrectObjectStateInterface
   virtual bool AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const override;
   
   // from ITATSmartObjectTagInterface
   virtual FGameplayTagCountContainer& GetGameplayTagCountContainer() override;
   
   FORCEINLINE ATATPowerSource* GetParentPowerSource() const { return _parentPowerSource.Get(); }

   DECLARE_MULTICAST_DELEGATE_OneParam(FOnPowerStateChanged, bool);
   // Called when supplied power state changes, due to self or parent power source being switched on/off
   FOnPowerStateChanged OnPowerStateChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPowerStateChangedDynamic);
   // Called when supplied power is restored/cut, whether due to self or a parent being switched on/off
   UPROPERTY(BlueprintAssignable, Category = "TAT|Electrical", meta = (DisplayName="OnPowerStateChanged"))
   FOnPowerStateChangedDynamic K2_OnPowerStateChanged;

   // Called when switched on/off, regardless of whether supplied power is restored/cut
   UPROPERTY(BlueprintAssignable, Category = "TAT|Electrical", meta = (DisplayName="OnPowerSwitched"))
   FOnPowerStateChangedDynamic K2_OnPowerSwitched;

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Interactable")
   void OnPoweredChanged(bool isPowered);

   // Called when the local on/off state is toggled via interaction. Use this for persistent or looping feedback
   UFUNCTION(BlueprintImplementableEvent, Category="TAT|Interactable")
   void OnToggled(bool bIsOn);

   // Called when the local on/off state is toggled via interaction. Use this to fire one-offs associated with the act of toggling
   UFUNCTION(BlueprintImplementableEvent, Category="TAT|Interactable")
   void OnRecentlyToggled(bool bIsOn);

   UFUNCTION(BlueprintImplementableEvent, Category="TAT|Interactable")
   void SyncTimelines(const FOSEToggleState& state);

   void Toggle();
protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;
   virtual void PostRegisterAllComponents() override;
   virtual void PostInitializeComponents() override;
#if WITH_EDITOR
   virtual void CheckForErrors() override;
   virtual void PreEditChange(FProperty* propertyThatWillChange) override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

   UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
   void _HandleAudibleTickFromResetTimer();
   UFUNCTION(BlueprintNativeEvent)
   void _HandleAutoResetWorldTimeChanged();

   UFUNCTION(BlueprintPure)
   float GetNormalizedTimeRemainingOnAutoReset() const;

   UPROPERTY(EditDefaultsOnly)
   UAIPerceptionStimuliSourceComponent* _perceptionStimuliSource = nullptr;
   
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|AI")
   UTATActionNodeComponent_IncorrectObjectState* _incorrectStateActionNodeComponent = nullptr;
private:
   UFUNCTION()
   void _OnRep_State(const FOSEToggleState& previousState);

   UFUNCTION()
   void _OnIsBrokenChanged(bool isBroken);

   void _OnIsPoweredChanged();

   UFUNCTION()
   void _OnParentPowerSourceIsPoweredChanged(bool isPowered);

#if WITH_EDITOR
   /// Returns true if the parent power source (or one of its ancestors) references this power source as a parent
   bool _PowerSourceChainHasCircularDependencyToSelf(const ATATPowerSource*& outSelfReferencingPowerSource) const;
   void _TrySetParentVisComponentDirty();
#endif // WITH_EDITOR

   bool _HasUnpoweredParentPowerSource() const;

   void _TryBindToParentPowerSource();
   void _TryUnbindFromParentPowerSource();
   
   UFUNCTION()
   void _OnSecurityLockdownStateChanged(bool bLockedDown);

#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   class UTATPowerSourceVisComponent* _visComponent = nullptr;
#endif // WITH_EDITORONLY_DATA

   UPROPERTY(EditAnywhere, Category="TAT|Security", meta=(DisplayPriority=1))
   bool _shouldAutoReset { false };

   UPROPERTY(EditAnywhere, Category="TAT|Security", meta=(DisplayPriority=1, EditCondition="_shouldAutoReset"))
   int _configTimeUntilAutoReset { 5 };
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Audio", meta=(DisplayPriority=1))
   UAkAudioEvent* _audibleTickFromResetTimerEvent { nullptr };

   FTimerHandle _autoResetHandle;
   int _timeRemainingUntilAutoReset { _configTimeUntilAutoReset };

   UFUNCTION()
   void OnRep_AutoResetWorldTimeChanged();
   UPROPERTY(Replicated, ReplicatedUsing=OnRep_AutoResetWorldTimeChanged)
   float _autoResetWorldTime { 0 };
   
   UFUNCTION()
   void _AuthorityOnAutoResetTimerTriggered();
   UFUNCTION(NetMulticast, Unreliable)
   void MulticastTriggerAudibleTickFromResetTimer();
   
   UPROPERTY(EditAnywhere, Category="TAT|Security", meta=(DisplayPriority=2))
   UTATSecurityLockdownComponent* _securityLockdownComponent = nullptr;

   UPROPERTY(VisibleAnywhere, Category="TAT|Security")
   TObjectPtr<UTATEndgameActionComponent> _endgameActionComponent = nullptr;
   
   UPROPERTY(EditAnywhere, ReplicatedUsing=_OnRep_State, Category = "TAT|Interactable", Meta = (AllowPrivateAccess = "true"))
   FOSEToggleState _state;

   UPROPERTY(EditInstanceOnly, Category = "TAT|Electrical")
   TWeakObjectPtr<ATATPowerSource> _parentPowerSource;

   // Player facing prompt for turning on the power
   UPROPERTY(EditDefaultsOnly, Category="TAT|Electrical", Meta = (AllowPrivateAccess = "true"))
   FText _turnOnPrompt;

   // Player facing prompt for turning off the power
   UPROPERTY(EditDefaultsOnly, Category="TAT|Electrical", Meta = (AllowPrivateAccess = "true", DisplayAfter="_turnOnPrompt"))
   FText _turnOffPrompt;

   // Whether the device should be toggled remotely by tools
   UPROPERTY(EditAnywhere, Category="TAT", meta = (AllowPrivateAccess = "true"))
   bool _toggleableByFairy = true;

   // Cached powered state, takes into account all influencing factors (power supplied by parent, broken, etc)
   bool _isPowered = false;

   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<USceneComponent> _rootComponent { nullptr };

   void _SetSwitchedOn(bool switchedOn, bool writeChangedTime = true);
   void _RefreshIsPowered();
};
