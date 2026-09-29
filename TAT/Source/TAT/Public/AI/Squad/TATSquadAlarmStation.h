// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/SmartObjects/TATActionNodeComponent.h"
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"
#include "Breakables/TATBreakableBase.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "Interactables/TATLockConfig.h"
#include "Lockpicking/LockpickableInterface.h"
#include "Traps/TATTrapActionInterface.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "Character/OSETeamInterface.h"

// ue4
#include "Perception/AISightTargetInterface.h"
#include "GameplayEffectTypes.h"
#include "NativeGameplayTags.h"

#include "TATSquadAlarmStation.generated.h"

class ATATAIContextualLocation;
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AlarmStation_SmartObject_Activity)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AlarmStation_Reset_SmartObject_Activity)

DECLARE_LOG_CATEGORY_EXTERN(LogTATSquadAlarmStation, Log, All);

class UTATMapActorComponent;
class UTATEndgameActionComponent;
struct FTATAuthorityBreakContext;

UENUM(BlueprintType, meta = (ScriptName = "AlarmStateType"))
enum class EAlarmState : uint8
{
   // Alarm is ready to activate.
   Ready,
   // The alarm has been triggered and is sounding.
   Triggered,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTATSquadAlarmStateChanged, EAlarmState, newState, EAlarmState, previousState, bool, isRecent);

USTRUCT(BlueprintType)
struct FAlarmState
{
   GENERATED_BODY()

   // Server time of the last time that the toggle was changed
   UPROPERTY(Transient)
   float ChangedServerTime = 0;

   UPROPERTY(EditAnywhere)
   EAlarmState State = EAlarmState::Ready;
};

UCLASS(Blueprintable)
class TAT_API ATATSquadAlarmStation
   : public AActor
   , public IAISightTargetInterface
   , public ILockpickableInterface
   , public IInteractableInterface
   , public ITATSmartObjectTagInterface
   , public ITATSmartObjectOwnerInterface
   , public IOSETeamInterface
   , public IGameplayTagAssetInterface
   , public ITATTrapActionInterface
{
   GENERATED_BODY()

   ATATSquadAlarmStation();
   
public:
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|OSE|Alarms")
   virtual void AuthorityTrigger();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|OSE|Alarms")
   virtual void AuthorityArm();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|OSE|Alarms")
   EAlarmState GetState() const { return _alarmState.State; }

   const bool IsInState(EAlarmState state) const { return GetState() == state; }
   bool IsReady() const;

   UPROPERTY(BlueprintAssignable, Category = "AI|OSE|Alarms")
   FTATSquadAlarmStateChanged OnAlarmStateChanged;

   // from IOSETeamInterface
   virtual uint8 GetTeam() const override;

   virtual void BeginPlay() override;
   // InteractableInterface from parent start
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;
   FLockInteractContext MakeLockContext(const ACharacter* interactingCharacter) const;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   // InteractableInterface end

   // IAISightTargetInterface start
   virtual bool CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor = nullptr,
      const bool* wasVisible = nullptr, int32* userData = nullptr) const override;
   // IAISightTargetInterface end

   // ITATSmartObjectTagInterface start
   virtual FGameplayTagCountContainer& GetGameplayTagCountContainer() override { return _gameplayTagCountContainer; }
   // ITATSmartObjectTagInterface end

   // ITATSmartObjectOwnerInterface start
   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override { return _actionNodeComponent; }
   // ITATSmartObjectOwnerInterface end

   // IGameplayTagAssetInterface start
   virtual bool HasMatchingGameplayTag(FGameplayTag tagToCheck) const override;
   virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override;
   virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override;
   virtual void GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const override;
   // IGameplayTagAssetInterface end

   // Begin ILockpickableInterface
   virtual void Unlock() override;
   virtual void Lock() override;
   virtual void LockWithKey() override { Lock(); };
   virtual void OnLockpickTrackCompleted(int32 trackIndex) override;
   virtual int32 GetLockpickCurrentTrack() override { return _lockpickCurrentTrack; }
   virtual FTATOnRequestCancelLockpicking* GetOnRequestCancelLockpickingDelegate() override { return &_onRequestCancelLockpicking; }
   // End ILockpickableInterface
   FORCEINLINE void AuthoritySetPrivateZoneTag(const FGameplayTag privateZoneTag) { _gameplayTagCountContainer.SetTagCount(privateZoneTag, 1); }


   // Begin ITATTrapActionInterface
   virtual void TriggerActionFromTrap_Implementation(AActor* optionalTarget) override;
   // End ITATTrapActionInterface
   
   const TArray<TObjectPtr<ATATAIContextualLocation>>& GetLockdownLocations() const { return _lockdownLocationsToSearch; }
protected:
   FTATOnRequestCancelLockpicking _onRequestCancelLockpicking;
   
   UPROPERTY(Replicated, Transient)
   int32 _lockpickCurrentTrack = 0;;
   
   UPROPERTY(EditDefaultsOnly, Category = "Lock")
   float _HoldTimeToDisableAlarm { 5.f };
   
   UPROPERTY(EditAnywhere, Category="Lock")
   FTATLockConfig _lockConfig;
   
   /// The state has changed, but it may have changed a long time ago
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnStateChanged", meta = (BlueprintProtected, ScriptName = "OnStateChanged"))
   void K2_OnAlarmStateChanged(EAlarmState newState, EAlarmState previousState, bool wasRecent);

   /// The state has changed recently. This is a good place to play ephemeral things like SFX for things like resetting
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnStateChangedRecently", meta = (BlueprintProtected, ScriptName = "OnStateChangedRecently"))
   void K2_OnAlarmStateChangedRecently(EAlarmState newState, EAlarmState previousState);

   UFUNCTION()
   void _OnRep_AlarmState(const FAlarmState& previousState);

   UFUNCTION()
   void OnResetAlarm();
   
   virtual void _SetState(EAlarmState newState);
   
   void _SetTagsForState(EAlarmState state);
   void _TriggerDelegates(bool isRecent, const FAlarmState& previousState);

   UPROPERTY(EditDefaultsOnly)
   class UAIPerceptionStimuliSourceComponent* _perceptionStimuliSource = nullptr;

   UPROPERTY(EditDefaultsOnly, Category="TAT|AI", BlueprintReadOnly)
   UTATActionNodeComponent* _actionNodeComponent { nullptr };

   UPROPERTY(VisibleAnywhere, Category="TAT", BlueprintReadOnly)
   TObjectPtr<UTATEndgameActionComponent> _endgameActionComponent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Interaction", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag ArmingAnimationTag;
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Interaction", meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag TriggeringAnimationTag;
   UPROPERTY(EditAnywhere, Category = "TAT|Interaction")
   FGameplayTag InteractionStatusTag;

   FGameplayTagCountContainer _gameplayTagCountContainer {};

   UPROPERTY(EditDefaultsOnly, Category="TAT")
   bool ShouldForceResetOnAlarm { false };
protected:
   UFUNCTION(BlueprintNativeEvent)
   void _OnTriggerAlarmStim();
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Tuning", meta=(Units="seconds"))
   float _TimeBetweenStimsBeingSent {1.f};

   // This is the number of second AFTER the alarm is flagged ready to reset for the guards
   UPROPERTY(EditDefaultsOnly, Category="TAT|Tuning", meta=(Units="seconds"))
   float _TimeUntilAlarmForcedToReset {1.f};
   
   FTimerHandle _triggeredAlarmRepeatStimHandle;
   FTimerHandle _alarmForcedResetHandle;
private:
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_AlarmState)
   FAlarmState _alarmState;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Map")
   UTATMapActorComponent* _mapActorComponent { nullptr };

   UPROPERTY(EditInstanceOnly, Category="TAT|Map")
   TArray<TObjectPtr<ATATAIContextualLocation>> _lockdownLocationsToSearch;
};
