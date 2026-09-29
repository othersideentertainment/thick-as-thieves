// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Breakables/TATBreakableActorFwd.h"
#include "Breakables/TATBreakableActorInfoInterface.h"
#include "Environment/TATInhibitableInterface.h"
#include "GameFramework/TATDifficulty.h"
#include "Settings/TATMatchSettingsBase.h"
#include "Character/TATTeams.h"

// ose
#include "AI/Perception/OSEAISightInterface.h"
#include "Character/OSETeamInterface.h"
#include "Interactables/OSEInteractableToggle.h"
#include "AI/Perception/OSEAITargetSightInterface.h"

// ue
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "TATVisionPerceptionDevice.generated.h"

enum class EAlarmState : uint8;
class ATATSquadAlarmStation;
class UTATAIPerceptionComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTATVisionPerceptionDevice, Warning, All);

UENUM(BlueprintType)
enum class ETATVisionPerceptionDeviceRotationType : uint8
{
   Swivel,
   ThreeSixtyClockwise,
   ThreeSixtyCounterClockwise
};

UENUM(BlueprintType)
enum class ETATVisionPerceptionDeviceState : uint8
{
   Normal,
   DetectedFriendly,
   Warning,
   Triggered,
   Suppressed,
   Destroyed,
   PausedAfterTrigger
};

UENUM(BlueprintType)
enum class ETATVisionPerceptionDeviceDetectSuspiciousActorBehavior : uint8
{
   KeepOscillating,
   PauseOscillation,
   TrackFirstDetected
};

UENUM(BlueprintType)
enum class ETATVisionPerceptionDeviceSwivelAxes : uint8
{
   Pitch,
   Yaw,
};

UENUM(BlueprintType, meta=(Bitflags))
enum class ETATVisionPerceptionDeviceTrackingRestrictions : uint8
{
   None = 0,
   AllowRoll = 1 << 0,
   AllowYaw = 1 << 1,
   AllowPitch = 1 <<2
};
ENUM_CLASS_FLAGS(ETATVisionPerceptionDeviceTrackingRestrictions);

UENUM(BlueprintType)
enum class ETATVisionPerceptionDeviceMovementType : uint8
{
   Oscillate,
   Track
};

USTRUCT()
struct FTATVisionPerceptionDeviceOscillationState
{
   GENERATED_BODY()

   /// If true, then the device should oscillate, otherwise it should pause in-place
   UPROPERTY()
   ETATVisionPerceptionDeviceMovementType MovementType { ETATVisionPerceptionDeviceMovementType::Oscillate };

   /// The time it was paused it according to the server
   UPROPERTY()
   float ServerTimeWhenFullyReset = 0.0f;

   UPROPERTY()
   float ServerTimeWhenFullyTracksActor = 0.f;
   
   UPROPERTY()
   float ServerWorldTimeOffset = 0.f;
   
   UPROPERTY()
   float ServerWorldTimeWhenStopped = 0.f;

   UPROPERTY(Transient)
   TObjectPtr<AActor> TrackedActor;
};

USTRUCT()
struct FTATVisionPerceptionDeviceDifficultyTuningSettings
{
   GENERATED_BODY()


   UPROPERTY(EditAnywhere, Category = "Swivel")
   bool UseMultiplierInsteadOfSeconds { false };
   
   UPROPERTY(EditAnywhere, Category="Detection", meta=(EditCondition="UseMultiplierInsteadOfSeconds", EditConditionHides, ClampMin=0.f, ClampMax=1.f))
   float SecondsUntilAlarmTriggersMultiplier{1.f};
   UPROPERTY(EditAnywhere, Category = "Swivel", meta=(EditCondition="UseMultiplierInsteadOfSeconds", EditConditionHides, ClampMin=0.f, ClampMax=1.f))
   float SwivelSecondsMultiplier { 1.f };
   UPROPERTY(EditAnywhere, Category = "Swivel", meta=(EditCondition="UseMultiplierInsteadOfSeconds", EditConditionHides, ClampMin=0.f, ClampMax=1.f))
   float SwivelPauseSecondsMultiplier { 1.f };
   UPROPERTY(EditAnywhere, Category = "Swivel", meta=(EditCondition="UseMultiplierInsteadOfSeconds", EditConditionHides, ClampMin=0.f, ClampMax=1.f))
   float SwivelResetTimeMultiplier { 1.f };
   
   UPROPERTY(EditAnywhere, Category="Detection", meta=(EditCondition="UseMultiplierInsteadOfSeconds == false", EditConditionHides))
   float SecondsUntilAlarmTriggers{3.f};
   
   /// The number of seconds to complete a back-and-forth of swiveling, not including any pauses at the extremes
   /// The total time for a cycle is `_swivelSeconds + _swivelPauseSeconds`
   UPROPERTY(EditAnywhere, Category = "Swivel", meta=(EditCondition="UseMultiplierInsteadOfSeconds == false", EditConditionHides))
   float SwivelSeconds{10.0f};
   /// The number of seconds to pause at both extremes of the swivel, in total for one cycle
   /// The total time for a cycle is `_swivelSeconds + _swivelPauseSeconds`
   UPROPERTY(EditAnywhere, Category = "Swivel", meta=(EditCondition="UseMultiplierInsteadOfSeconds == false", EditConditionHides))
   float SwivelPauseSeconds{0.0f};

   UPROPERTY(EditAnywhere, Category = "Swivel", meta=(EditCondition="UseMultiplierInsteadOfSeconds == false", EditConditionHides))
   float SwivelResetTime{1.0f};
};

USTRUCT()
struct FTATVisionPerceptionLodThreshold
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, meta=(Units="Cm"))
   float MaxDistanceToViewer = 0;
   UPROPERTY(EditDefaultsOnly)
   float TickInterval = 0;
};

UCLASS()
class TAT_API ATATVisionPerceptionDevice
   : public AOSESyncedToggle
   , public IOSEAISightInterface
   , public IOSETeamInterface
   , public IAbilitySystemInterface
   , public IGameplayTagAssetInterface
   , public ITATInhibitableInterface
   , public ITATBreakableActorInfoInterface //< breakable
   , public IOSEAITargetSightInterface
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeviceStateChanged, ETATVisionPerceptionDeviceState, deviceState, ETATVisionPerceptionDeviceState, oldDeviceState);

public:
   ATATVisionPerceptionDevice();

   /// from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void Tick(float deltaSeconds) override;
   virtual void GetActorEyesViewPoint(FVector& outLocation, FRotator& outRotation) const override;

   /// from IOSEAISightInterface
   virtual bool IsAllowedToSeeActor(const AActor* actor) const override;
   virtual void ModifySightRangeForSpecificActor(const AActor* actor, float& outSightRadius) const override;

   /// from AOSESyncedToggle
   virtual void SetOn(bool newOn) override;
   virtual void _OnStateChanged(bool bIsOn, bool bWasRecent) override;
   virtual bool CanSwitchState_Implementation() const override;

   /// from IOSETeamInterface
   virtual uint8 GetTeam() const override;
   
   // begin IOSEAITargetSightInterface
   virtual EAISightBucket GetBucketForTarget_Implementation() const override { return EAISightBucket::Medium; }
   // end IOSEAITargetSightInterface

   /// from ITATInhibitableInterface
   virtual bool CanBeInhibitedBy_Implementation(FGameplayTag inhibitorType) const override { return IsInhibitable; }
   virtual FGameplayTag GetInhibitableType_Implementation() const override { return InhibitableType; }
   virtual FTATInhibitorPlacementInfo GetInhibitorPlacementInfo_Implementation() const override;
   virtual void OnInhibitorActivated_Implementation(ATATInhibitorActor* inhibitorActor, APawn* instigator, int32 newInhibitorCount) override {}
   virtual void OnInhibitorDeactivated_Implementation(ATATInhibitorActor* inhibitorActor, int32 newInhibitorCount, bool allInhibitorsRemoved) override {}

   const TArray<FTATVisionPerceptionLodThreshold>& GetLodThresholds() const { return _significanceThresholds; }

   BREAKABLE_ACTOR_DECLARATIONS()

   UFUNCTION(BlueprintPure, Category = "Vision Perception Device")
   UTATAIPerceptionComponent* GetPerceptionComponent() const { return _perceptionComponent; }

   UFUNCTION(BlueprintPure, Category = "Vision Perception Device")
   const TArray<ATATSquadAlarmStation*>& GetAlarmStations() const { return AlarmStations; }

   UFUNCTION(BlueprintPure, Category = "Vision Perception Device")
   void CalculateAverageLocationOfPerceivedActors(int& numActorsPerceived, FVector& averageActorLocation) const;

   // External event for device state changes
   UPROPERTY(BlueprintAssignable, Category = "Vision Perception Device")
   FOnDeviceStateChanged OnDeviceStateChanged;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision Perception Device")
   ETATTeamDisguiseHandling DisguiseHandling = ETATTeamDisguiseHandling::UseOriginalTeam;

   /// If the swivel is currently enabled: set to false to pause it in-place
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swivel")
   bool SwivelEnabled = true;

   /// Which component is meant to be swiveled: set this in the Blueprint's construction script
   UPROPERTY(BlueprintReadWrite, Category = "Swivel")
   USceneComponent* SwivelPivotComponent = nullptr;

   /// Allow inhibitors to work on this device
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable")
   bool IsInhibitable = false;

   /// The type/category of this device in terms of things that can be inhibited in the world.
   /// Allows things that apply inhibitors to decide what they can inhibit (eg. a tool that can inhibit this actor only if it has the type "Inhibitable.SecurityDevice")
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable", Meta = (EditCondition = "IsInhibitable", Categories = "Inhibitable"))
   FGameplayTag InhibitableType;

#if WITH_EDITOR
   // from AActor
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR

protected:
   UFUNCTION()
   void _OnMatchSettingsUpdated(UTATMatchSettingsBase* newMatchSettings);

   virtual void SetSettingsForDifficulty(const ETATDifficulty difficulty);
   
   // Internal event for device state changes
   UFUNCTION(BlueprintNativeEvent, Category = "Vision Perception Device")
   void _OnDeviceStateChanged(ETATVisionPerceptionDeviceState deviceState, ETATVisionPerceptionDeviceState oldDeviceState);
   virtual void _OnDeviceStateChanged_Implementation(ETATVisionPerceptionDeviceState deviceState, ETATVisionPerceptionDeviceState oldDeviceState) { }

   // Internal event for device triggering an alarm due to a player in it's sights
   UFUNCTION(BlueprintNativeEvent, Category = "Vision Perception Device")
   void _OnDeviceTriggeredAlarm();
   void _OnDeviceTriggeredAlarm_Implementation() {}

   // where should the "eyes" be for the sight perception?
   UFUNCTION(BlueprintNativeEvent, Category = "Vision Perception Device")
   USceneComponent* _GetEyesViewpointComponent() const;
   USceneComponent* _GetEyesViewpointComponent_Implementation() const { return nullptr; }

   UPROPERTY(EditInstanceOnly, Category = "Vision Perception Device")
   TArray<ATATSquadAlarmStation*> AlarmStations;
   
   UPROPERTY(EditDefaultsOnly, Category="Vision Perception Device")
   bool _bShouldTriggerAlarm { false };
   void _AuthorityTickDeviceState();
   void _AuthorityTryTriggerAlarms();

   UFUNCTION()
   void _AuthorityTargetSightPerceptionUpdated(AActor* actor, FAIStimulus stimulus);
   UFUNCTION()
   virtual void _AuthorityOnAlarmSystemStateChanged(EAlarmState newState, EAlarmState previousState, bool isRecent);
   
   UFUNCTION()
   void _AuthorityOnBrokenChanged(bool isBroken);
   virtual bool _ShouldTurnOnWhenRepaired() const;
   
   UFUNCTION()
   void _OnRep_DeviceState(ETATVisionPerceptionDeviceState oldDeviceState);
   void _BroadcastDeviceStateChanged(ETATVisionPerceptionDeviceState oldDeviceState);
   void _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState deviceState);

   UFUNCTION()
   void _OnRep_DeviceOscillationState(const FTATVisionPerceptionDeviceOscillationState& oldState);
   FQuat _GetDesiredQuatForNormalizedProgress(float swivelProgress) const;
   void _AuthoritySyncOscillationState();
   void _SetSwivelForTime(float time);
   /// 0.0 - the near extreme of the swivel
   /// 1.0 - the far extreme of the swivel
   void _SetSwivelNormalizedProgress(float swivelProgress) const;
   void _SetSwivelForTarget(float worldTime, const AActor* target) const;
   
   float _GetSwivelTimeProgressForRotation(float rotation) const;
   float _GetSwivelProgressForTime(float time) const;

   bool _ShouldPauseFromDetection() const;
   void _CheckIfSuspiciousActorsSeenChanged();
   void _FilterOutDeadSeenActors();

   enum class ETrackedActorStatus : uint8
   {
      ConsciousFriendly,
      UnconsciousFriendly,
      Enemy,
      Untracked
   };

   struct FTrackedActor
   {
      TWeakObjectPtr<AActor> actor;
      ETrackedActorStatus status = ETrackedActorStatus::Untracked;
   };

   void _UpdateTrackedActorStatuses();
   ETrackedActorStatus _GetStatusOfTrackedActor(AActor* actor) const;
   bool _IsDetectingEnemyActor() const { return _bIsDetectingEnemyActor; }
   bool _IsDetectingSuspiciousActor() const { return _bIsDetectingSuspiciousActor; }
   bool _IsDetectingFriendlyActors() const{ return _bIsDetectingFriendlyActor; }

   bool _bIsDetectingEnemyActor { false };
   bool _bIsDetectingSuspiciousActor { false };
   bool _bIsDetectingFriendlyActor { false };

   EOSETeamAttitude _GetAttitudeTowardsActor(const AActor* actor) const;

   // Breakable boilerplate
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UAbilitySystemComponent> _abilitySystemComponent = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TObjectPtr<UTATBreakableComponent> _breakableComponent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Vision Perception Device")
   UTATAIPerceptionComponent* _perceptionComponent = nullptr;

   UPROPERTY(Transient)
   USceneComponent* _eyesViewpointComponent = nullptr;

   UPROPERTY(Transient, BlueprintReadOnly, ReplicatedUsing = _OnRep_DeviceState)
   ETATVisionPerceptionDeviceState _deviceState = ETATVisionPerceptionDeviceState::Normal;

   UPROPERTY(EditDefaultsOnly, Category = "Vision Perception Device")
   float _secondsUntilAlarmTriggers = 3.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Vision Perception Device")
   ETATVisionPerceptionDeviceDetectSuspiciousActorBehavior _detectSuspiciousActorBehavior = ETATVisionPerceptionDeviceDetectSuspiciousActorBehavior::KeepOscillating;

   // If set, this will override the SwivelSeconds, SwivelPauseSeconds and SwivelResetTime and _secondsUntilAlarmTriggers variables.
   UPROPERTY(EditAnywhere, Category = "TAT|Tuning")
   TMap<ETATDifficulty, FTATVisionPerceptionDeviceDifficultyTuningSettings> _difficultyToSwivelSettingsMap;
   
   /// The number of seconds to complete a back-and-forth of swiveling, not including any pauses at the extremes
   /// The total time for a cycle is `_swivelSeconds + _swivelPauseSeconds`
   UPROPERTY(EditAnywhere, Category = "Swivel")
   float _swivelSeconds = 10.0f;

   /// The number of seconds to pause at both extremes of the swivel, in total for one cycle
   /// The total time for a cycle is `_swivelSeconds + _swivelPauseSeconds`
   UPROPERTY(EditAnywhere, Category = "Swivel")
   float _swivelPauseSeconds = 0.0f;
   
   UPROPERTY(EditAnywhere, Category = "Swivel")
   float _swivelResetTime = 1.0f;

   /// The size of angle range that is covered by the swivel (in degrees)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swivel")
   float _swivelAngle = 30.0f;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swivel")
   ETATVisionPerceptionDeviceRotationType _swivelType { ETATVisionPerceptionDeviceRotationType::Swivel };
   
   UPROPERTY(EditAnywhere, Category = "Tracking")
   float _timeToFullyRotateCameraToLookAtTarget = 2.0f;
   
   UPROPERTY(EditAnywhere, Category = "Tracking")
   float _timeToFullyResetAfterTracking = 2.0f;

   UPROPERTY(EditDefaultsOnly, Category="AI|Light Detection")
   UCurveFloat* _LightIntensityToRangeMultiplierCurve { nullptr };
   
   UPROPERTY(EditAnywhere, Category = "Tracking")
   float _timeUntilPauseStateExitAfterTrigger = 2.0f;
   
   UPROPERTY(EditAnywhere, Category = "Swivel")
   ETATVisionPerceptionDeviceSwivelAxes _swivelAxes = ETATVisionPerceptionDeviceSwivelAxes::Yaw;

   
   UPROPERTY(EditAnywhere, Category = "Tracking", meta=(Bitmask, BitmaskEnum="/Script/TAT.ETATVisionPerceptionDeviceTrackingRestrictions"))
   uint8 _trackingRestrictions = 0;

   UPROPERTY(EditAnywhere, Category = "Tracking|Restrictions")
   float _trackingAngleRestrictionPitch = 30.f;
   UPROPERTY(EditAnywhere, Category = "Tracking|Restrictions")
   float _trackingAngleRestrictionYaw = 30.f;
   UPROPERTY(EditAnywhere, Category = "Tracking|Restrictions")
   float _trackingAngleRestrictionRoll = 30.f;
   
   /// If true, then the device will acknowledge friendlies, not triggered, but detecting enemies take precedence
   UPROPERTY(EditAnywhere, Category = "Vision Perception Device")
   bool _shouldReactToFriendlies = false;

   // NOTE: It might be nice to have this in an external data asset for easier tuning,
   //       but this is a simple thing to start with.
   UPROPERTY(EditDefaultsOnly, Category = "Tick", DisplayName="Significance Thresholds", meta=(TitleProperty="<{MaxDistanceToViewer}cm => {TickInterval}"))
   TArray<FTATVisionPerceptionLodThreshold> _significanceThresholds;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_DeviceOscillationState)
   FTATVisionPerceptionDeviceOscillationState _oscillationState;

   ETATVisionPerceptionDeviceMovementType _GetRequiredMovementType() const;
   FVector2D _GetSwivelOutputRangeForType() const;
   float _GetRotationFromSceneComponent() const;

   TArray<FTrackedActor, TInlineAllocator<4>> _trackedActors;

   float _suspiciousActorsSeenWorldTimeStart = static_cast<float>(INDEX_NONE);
   float _timeUntilPauseStateShouldReset = static_cast<float>(INDEX_NONE);

   FRotator _swivelInitialRotation;
};
