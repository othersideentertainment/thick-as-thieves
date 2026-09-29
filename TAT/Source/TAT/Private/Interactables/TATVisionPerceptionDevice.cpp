// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATVisionPerceptionDevice.h"

// tat
#include "AI/TATAISettings.h"
#include "AI/Squad/TATSquadAlarmStation.h"
#include "AI/Perception/TATAIPerceptionComponent.h"
#include "Breakables/TATBreakableActorImpl.h"
#include "Character/TATTeams.h"
#include "Developer/TATProjectSettings.h"
#include "Environment/TATPrivateSpaceGameplayTagDefines.h"
#include "AI/UnifiedStealthSystem/TATStealthScoreInterface.h"
#include "Environment/TATInhibitorSubsystem.h"
#include "TATGameInstance.h"

// ose 
#include "Character/OSECharacterBase.h"
#include "Character/OSETeamInterface.h"
#include "OSELightDetectionFunctionLibrary.h"
#include "OSELightDetectionInterface.h"

// ue
#include "SignificanceManager.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"
#include "Traps/TATTrapDetectorComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATVisionPerceptionDevice)

DEFINE_LOG_CATEGORY(LogTATVisionPerceptionDevice);

BREAKABLE_ACTOR_IMPLS(ATATVisionPerceptionDevice, _abilitySystemComponent, _breakableComponent)

namespace PerceptionDeviceSignificance
{
   static const FName kSignificanceTag = "PerceptionDeviceTick";

   static float SignificanceFunction(const USignificanceManager::FManagedObjectInfo* objectInfo, const FTransform& viewport)
   {
         const ATATVisionPerceptionDevice* actor = CastChecked<ATATVisionPerceptionDevice>(objectInfo->GetObject());
         const float distanceSquared = FVector::DistSquared(actor->GetActorLocation(), viewport.GetLocation());

         float interval = 0;
         for (const FTATVisionPerceptionLodThreshold& threshold : actor->GetLodThresholds())
         {
            interval = threshold.TickInterval;
            if (distanceSquared < FMath::Square(threshold.MaxDistanceToViewer))
            {
               break;
            }
         }

         // significance is sorted descending by default (will use the highest)
         // so just encoding the significance as the negative interval is an easy way
         // to achieve that. Other ways to do if if we need it to be positive, but not
         // sure that we care.
         return -interval;
   }

   static void PostSignificanceFunction(const USignificanceManager::FManagedObjectInfo* objectInfo,
      float oldSignificance,
      float significanceValue,
      bool bFinal)
   {
      if (oldSignificance != significanceValue && !bFinal)
      {
         // significance
         const float interval = -significanceValue;
         AActor* actor = CastChecked<AActor>(objectInfo->GetObject());
         if (interval > 0)
         {
            // Set the initial cooldown to a random value in the interval to reduce clustering
            // Setting the tick interval afterward will then set the tick interval when scheduled the next time
            actor->PrimaryActorTick.UpdateTickIntervalAndCoolDown(FMath::RandRange(0.f, interval));
         }
         actor->PrimaryActorTick.TickInterval = interval;
      }
   }
}

ATATVisionPerceptionDevice::ATATVisionPerceptionDevice()
   : Super()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
   bReplicates = true;

   _perceptionComponent = CreateDefaultSubobject<UTATAIPerceptionComponent>(TEXT("PerceptionComponent"));

   _abilitySystemComponent = CreateAbilitySystemForBreakables(this);
   _breakableComponent = CreateDefaultSubobject<UTATBreakableComponent>(TEXT("BreakableComponent"));
}

void ATATVisionPerceptionDevice::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      if (_perceptionComponent)
      {
         _perceptionComponent->OnTargetSightPerceptionUpdated.AddUniqueDynamic(this, &ATATVisionPerceptionDevice::_AuthorityTargetSightPerceptionUpdated);
      }

      for(ATATSquadAlarmStation* alarmStation : AlarmStations)
      {
         if (!IsValid(alarmStation))
            continue;

         alarmStation->OnAlarmStateChanged.AddUniqueDynamic(this, &ATATVisionPerceptionDevice::_AuthorityOnAlarmSystemStateChanged);
      }
      
      _breakableComponent->OnBrokenChanged.AddUniqueDynamic(this, &ThisClass::_AuthorityOnBrokenChanged);
      if(IsOn() == false)
      {
         _oscillationState.ServerWorldTimeWhenStopped = GetWorld()->GetTimeSeconds();
      }
   }

   if (_significanceThresholds.Num() > 0)
   {
      if (USignificanceManager* significanceManager = USignificanceManager::Get(GetWorld()))
      {
         significanceManager->RegisterObject(this, PerceptionDeviceSignificance::kSignificanceTag, PerceptionDeviceSignificance::SignificanceFunction, USignificanceManager::EPostSignificanceType::Sequential, PerceptionDeviceSignificance::PostSignificanceFunction);
      }
   }

   SetActorTickEnabled(true);

   UTATGameInstance* gi = GetWorld()->GetGameInstance<UTATGameInstance>();
   gi->OnMatchSettingsUpdated.AddDynamic(this, &ATATVisionPerceptionDevice::_OnMatchSettingsUpdated);

   _OnMatchSettingsUpdated(&gi->GetMatchSettings());
   _eyesViewpointComponent = _GetEyesViewpointComponent();
}

void ATATVisionPerceptionDevice::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   if (USignificanceManager* significanceManager = USignificanceManager::Get(GetWorld()))
   {
      significanceManager->UnregisterObject(this);
   }

   if (UTATGameInstance* gi = GetWorld()->GetGameInstance<UTATGameInstance>())
   {
      gi->OnMatchSettingsUpdated.RemoveAll(this);
   }
   Super::EndPlay(EndPlayReason);
}

void ATATVisionPerceptionDevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATVisionPerceptionDevice, _deviceState);
   DOREPLIFETIME(ATATVisionPerceptionDevice, _oscillationState);
}

void ATATVisionPerceptionDevice::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);
   if(IsOn() == false)
      return;
   if (SwivelEnabled && SwivelPivotComponent)
   {
      if (const AGameStateBase* gs = GetWorld()->GetGameState())
      {
         if(_oscillationState.MovementType == ETATVisionPerceptionDeviceMovementType::Oscillate)
         {
            const float worldTimeSeconds = gs->GetServerWorldTimeSeconds();
            const float worldTimeSecondsMinusPause = worldTimeSeconds + _oscillationState.ServerWorldTimeOffset;
            if(_oscillationState.ServerTimeWhenFullyReset > worldTimeSeconds)
            {
               const float timeUntilFullyReset = _oscillationState.ServerTimeWhenFullyReset - worldTimeSeconds;

               // if we are < swivelResetTime then we have finished with the pause
               if(timeUntilFullyReset < _swivelResetTime)
               {
                  const FRotator actualRot = SwivelPivotComponent->GetRelativeRotation();
                  const FRotator desiredRot = _GetDesiredQuatForNormalizedProgress(_GetSwivelProgressForTime(worldTimeSecondsMinusPause)).Rotator();
                  SwivelPivotComponent->SetRelativeRotation(
                     FMath::Lerp(desiredRot, actualRot,
                        FMath::GetMappedRangeValueClamped(
                              FVector2D(0, _swivelResetTime),
                              FVector2D(0, 1),
                              timeUntilFullyReset)
                              )
                     );
               }
            }
            else
            {
               _SetSwivelForTime(worldTimeSecondsMinusPause);
            }
         }
         else if(_oscillationState.MovementType == ETATVisionPerceptionDeviceMovementType::Track)
         {
            // When tracking a target, we have a different flow which tracks the target passed down from the server
            // and using a time offset which tells us when we should be "locked on" to the target, we do a simple lerp
            if(_oscillationState.TrackedActor)
            {
               _SetSwivelForTarget(gs->GetServerWorldTimeSeconds(), _oscillationState.TrackedActor);
            }
         }
      }
   }

   if (HasAuthority())
   {
      _AuthorityTickDeviceState();
   }
}

void ATATVisionPerceptionDevice::GetActorEyesViewPoint(FVector& outLocation, FRotator& outRotation) const
{
   if (_eyesViewpointComponent)
   {
      FTransform worldXfm = _eyesViewpointComponent->GetComponentTransform();
      outLocation = worldXfm.GetLocation();
      outRotation = worldXfm.Rotator();
   }
   else
   {
      Super::GetActorEyesViewPoint(outLocation, outRotation);
   }
}

bool ATATVisionPerceptionDevice::IsAllowedToSeeActor(const AActor* actor) const
{
   // If this actor is inhibited, don't allow it to see any actor
   if (IsInhibitable)
   {
      UTATInhibitorSubsystem* inhibitorSubsystem = GetWorld()->GetSubsystem<UTATInhibitorSubsystem>();
      if (inhibitorSubsystem && inhibitorSubsystem->IsActorCurrentlyInhibited(this))
      {
         return false;
      }
   }

   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(actor))
   {
      const UTATAISettings& settings = UTATAISettings::Get();
      if (tagInterface->HasAnyMatchingGameplayTags(settings.DevicesDoNotSeeTags))
         return false;
   }

   // TODO: OSE sight is still using the engine-level team affiliation and not our OSE-level affiliations; we should change that!
   //       Our normal AI does not care, but this thing only wants to see things that are hostile to the guard team
   const EOSETeamAttitude attitudeToActor = _GetAttitudeTowardsActor(actor);

   // always track enemies seen
   if (attitudeToActor == EOSETeamAttitude::Hostile)
   {
      return true;
   }
   // if set to react to friendlies, also track them
   else if (_shouldReactToFriendlies)
   {
      return attitudeToActor == EOSETeamAttitude::Friendly;
   }
   else
   {
      return false;
   }
}

void ATATVisionPerceptionDevice::ModifySightRangeForSpecificActor(const AActor* actor, float& outSightRadius) const
{
   if(UTATProjectSettings::ShouldUseLightDetection() == false)
      return;
   if(const ITATStealthScoreInterface* stealthScoreInterface = Cast<ITATStealthScoreInterface>(actor))
   {
      outSightRadius *= UOSELightDetectionFunctionLibrary::GetRangeMultiplierFromLightIntensity(
         1.f - stealthScoreInterface->GetStealthScore(),
         _LightIntensityToRangeMultiplierCurve
      );
   }
}

void ATATVisionPerceptionDevice::_AuthorityOnBrokenChanged(bool isBroken)
{
   check(HasAuthority())
   if (isBroken)
   {
      _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState::Destroyed);
      SetOn(false);
   }
   else
   {
      _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState::Normal);
      SetOn(_ShouldTurnOnWhenRepaired());
   }
}

bool ATATVisionPerceptionDevice::_ShouldTurnOnWhenRepaired() const
{
   return true;
}

void ATATVisionPerceptionDevice::SetOn(const bool newOn)
{
   if(IsOn() == newOn) return;

   Super::SetOn(newOn);

   if (HasAuthority())
   {
      if(newOn == false)
      {
         _oscillationState.ServerWorldTimeWhenStopped = GetWorld()->GetTimeSeconds();
      }
      else
      {
         _oscillationState.ServerWorldTimeOffset += _oscillationState.ServerWorldTimeWhenStopped - GetWorld()->GetTimeSeconds();
      }
      // disable oscillation when the device is off, re-enable it once it's on again
      _AuthoritySyncOscillationState();

      if (!newOn)
      {
         _trackedActors.Reset();
         _suspiciousActorsSeenWorldTimeStart = float(INDEX_NONE);
      }
   }
}

void ATATVisionPerceptionDevice::_OnStateChanged(bool bIsOn, bool bWasRecent)
{
   Super::_OnStateChanged(bIsOn, bWasRecent);

   // this isn't gated by a change to the State.bIsOn flag
   // and is called in AOSESyncedToggle::BeginPlay so we'll
   // make sure to get the initial value
   if (HasAuthority())
   {
      // disable perception when the light is disabled
      if (_perceptionComponent)
      {
         _perceptionComponent->SetAllSensesEnabled(bIsOn);
      }
   }
}

bool ATATVisionPerceptionDevice::CanSwitchState_Implementation() const
{
   if(_deviceState == ETATVisionPerceptionDeviceState::Suppressed)
   {
      return false;
   }

   // don't allow switching on if broken
   if(!IsOn() && _breakableComponent->IsBroken())
   {
      return false;
   }

   return true;
}

FTATInhibitorPlacementInfo ATATVisionPerceptionDevice::GetInhibitorPlacementInfo_Implementation() const
{
   // Child classes will almost certainly want to override this, but we can provide a simple baseline implementation here
   return FTATInhibitorPlacementInfo::Make(GetActorLocation(), GetActorRotation());
}

void ATATVisionPerceptionDevice::CalculateAverageLocationOfPerceivedActors(int& numActorsPerceived, FVector& averageActorLocation) const
{
   numActorsPerceived = 0;
   averageActorLocation = FVector::ZeroVector;

   if (!_perceptionComponent)
      return;

   TArray<AActor*> sensedActors; // would prefer that this was an inline alloc of size 4, but at least the utl function reserves under the hood?
   _perceptionComponent->GetFilteredActors([](const FActorPerceptionInfo& actorPerceptionInfo)
   {
      return (actorPerceptionInfo.HasAnyKnownStimulus());
   }, sensedActors);

   numActorsPerceived = sensedActors.Num();
   if (numActorsPerceived == 0)
      return;

   averageActorLocation = UGameplayStatics::GetActorArrayAverageLocation(sensedActors);
}

#if WITH_EDITOR
void ATATVisionPerceptionDevice::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      FMessageLog msgLog(FName("MapCheck"));

      for (ATATSquadAlarmStation* alarmStation : AlarmStations)
      {
         if (!IsValid(alarmStation))
         {
            msgLog.Error()
               ->AddToken(FUObjectToken::Create(this))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Vision Perception Device \"%s\" has an invalid alarm station entry!"), *GetName()))));
         }
      }
   }
}
#endif // WITH_EDITOR

void ATATVisionPerceptionDevice::_OnMatchSettingsUpdated(UTATMatchSettingsBase* newMatchSettings)
{
   const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
   SetSettingsForDifficulty(currentDifficulty);
}

void ATATVisionPerceptionDevice::SetSettingsForDifficulty(const ETATDifficulty difficulty)
{
   if (const FTATVisionPerceptionDeviceDifficultyTuningSettings* settings = _difficultyToSwivelSettingsMap.
      Find(difficulty))
   {
      if (settings->UseMultiplierInsteadOfSeconds)
      {
         _swivelPauseSeconds = _swivelPauseSeconds * settings->SwivelPauseSecondsMultiplier;
         _swivelSeconds = _swivelSeconds * settings->SwivelSecondsMultiplier;
         _swivelResetTime = _swivelResetTime * settings->SwivelResetTimeMultiplier;
         _secondsUntilAlarmTriggers = _secondsUntilAlarmTriggers * settings->SecondsUntilAlarmTriggersMultiplier;
      }
      else
      {
         _swivelPauseSeconds = settings->SwivelPauseSeconds;
         _swivelSeconds = settings->SwivelSeconds;
         _swivelResetTime = settings->SwivelResetTime;
         _secondsUntilAlarmTriggers = settings->SecondsUntilAlarmTriggers;
      }
   }
}

void ATATVisionPerceptionDevice::_OnRep_DeviceOscillationState(const FTATVisionPerceptionDeviceOscillationState& oldState)
{

}

FQuat ATATVisionPerceptionDevice::_GetDesiredQuatForNormalizedProgress(const float swivelProgress) const
{
   checkf(swivelProgress >= 0.0f && swivelProgress <= 1.0f, TEXT("_GetDesiredQuatForNormalizedProgress called out of range: %f"), swivelProgress);

   float currentSwivelValue = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f),
                                                              _GetSwivelOutputRangeForType(),
                                                              swivelProgress);

   if(_swivelType != ETATVisionPerceptionDeviceRotationType::Swivel)
   {
      const FVector2D outputRange = _GetSwivelOutputRangeForType();
      currentSwivelValue = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f),
                                                              outputRange,
                                                              swivelProgress);
   }
   FRotator currentSwivelRelativeRotator;
   switch(_swivelAxes)
   {
   case ETATVisionPerceptionDeviceSwivelAxes::Pitch:
      {
         currentSwivelRelativeRotator = FRotator(currentSwivelValue, 0.0f, 0.0f);
      }
      break;
   case ETATVisionPerceptionDeviceSwivelAxes::Yaw:
   default:
      {
         currentSwivelRelativeRotator = FRotator(0.0f, currentSwivelValue, 0.0f);
      }
      break;
   }
   return currentSwivelRelativeRotator.Quaternion();
}

void ATATVisionPerceptionDevice::_SetSwivelNormalizedProgress(float swivelProgress) const
{
   SwivelPivotComponent->SetRelativeRotation(_GetDesiredQuatForNormalizedProgress(swivelProgress));
}

void ATATVisionPerceptionDevice::_SetSwivelForTarget(float worldTime, const AActor* target) const
{
   QUICK_SCOPE_CYCLE_COUNTER(TATVisionPerceptionDevice_Swivel_ForTarget);
   if (SwivelEnabled && SwivelPivotComponent && target)
   {
      const FVector targetPos = target->GetActorLocation();

      // We know the time that we should be fully rotated and facing the target position, so we map the current time value
      // to a normalized 0-1, then use that to lerp from the current rotation -> tracked target rotation
      const float timeUntilFullyTracked = FMath::GetMappedRangeValueClamped(
         FVector2D(0.f, _timeToFullyRotateCameraToLookAtTarget),
         FVector2D(0.f, 1.f),
         _oscillationState.ServerTimeWhenFullyTracksActor - worldTime);

      const FRotator currentRotation = SwivelPivotComponent->GetRelativeRotation();
      const FQuat currentRotationQuat = currentRotation.Quaternion();
      const FQuat targetWorldRotation = FRotationMatrix::MakeFromX(targetPos - SwivelPivotComponent->GetComponentLocation()).ToQuat();
      const FQuat targetRotation = SwivelPivotComponent->GetRelativeRotationFromWorld(targetWorldRotation);
      const FQuat rotationQuat = FMath::Lerp(currentRotationQuat, targetRotation, 1.f - timeUntilFullyTracked);

      const FRotator rotation = rotationQuat.Rotator();

      auto clampUnwoundAngle = [](float angle, float limit) {
         return FMath::Clamp(FMath::UnwindDegrees(angle), -limit, limit);
      };

      FRotator outputRotation;
      if (EnumHasAnyFlags(static_cast<ETATVisionPerceptionDeviceTrackingRestrictions>(_trackingRestrictions),
                          ETATVisionPerceptionDeviceTrackingRestrictions::AllowPitch)
         )
      {
         if(_swivelType != ETATVisionPerceptionDeviceRotationType::Swivel)
         {
            outputRotation.Pitch = rotation.Pitch;
         }
         else
         {
            const float clampLimit = _swivelAxes == ETATVisionPerceptionDeviceSwivelAxes::Pitch ? _swivelAngle : _trackingAngleRestrictionPitch;
            outputRotation.Pitch = clampUnwoundAngle(rotation.Pitch, clampLimit);
         }
      }
      else
      {
         outputRotation.Pitch = currentRotation.Pitch;
      }
     
      if (EnumHasAnyFlags(static_cast<ETATVisionPerceptionDeviceTrackingRestrictions>(_trackingRestrictions),
                          ETATVisionPerceptionDeviceTrackingRestrictions::AllowYaw)
         )
      {
         if(_swivelType != ETATVisionPerceptionDeviceRotationType::Swivel)
         {
            outputRotation.Yaw = rotation.Yaw;
         }
         else
         {
            const float clampLimit = _swivelAxes == ETATVisionPerceptionDeviceSwivelAxes::Yaw ? _swivelAngle : _trackingAngleRestrictionYaw;
            outputRotation.Yaw = clampUnwoundAngle(rotation.Yaw, clampLimit);
         }
      }
      else
      {
         outputRotation.Yaw = currentRotation.Yaw;
      }
      
      if (EnumHasAnyFlags(static_cast<ETATVisionPerceptionDeviceTrackingRestrictions>(_trackingRestrictions),
                          ETATVisionPerceptionDeviceTrackingRestrictions::AllowRoll))
      {
         if(_swivelType != ETATVisionPerceptionDeviceRotationType::Swivel)
         {
            outputRotation.Roll = rotation.Roll;
         }
         else
         {
            outputRotation.Roll = clampUnwoundAngle(rotation.Roll, _trackingAngleRestrictionRoll);
         }
      }
      else
      {
         outputRotation.Roll = currentRotation.Roll;
      }
      SwivelPivotComponent->SetRelativeRotation(outputRotation);
   }
}

float ATATVisionPerceptionDevice::_GetSwivelTimeProgressForRotation(const float rotation) const
{
   const float wholeCycleTime = _swivelSeconds + _swivelPauseSeconds;
   const float swivelTime = FMath::GetMappedRangeValueClamped(_GetSwivelOutputRangeForType(),
                                                              FVector2D(0.f, wholeCycleTime), rotation);
   return swivelTime;
}

float ATATVisionPerceptionDevice::_GetSwivelProgressForTime(const float time) const
{
   if(_swivelType != ETATVisionPerceptionDeviceRotationType::Swivel)
   {
      const float modTime = FMath::Fmod(time, _swivelSeconds);
      return FMath::Clamp(modTime / _swivelSeconds, 0.f, 1.f);
   }
   const float halfSwivelTime = _swivelSeconds * 0.5f;
   const float halfPauseTime = _swivelPauseSeconds * 0.5f;
   const float wholeCycleTime = _swivelSeconds + _swivelPauseSeconds;
   const float currentCycleTime = FMath::Fmod(time + halfSwivelTime * 0.5, wholeCycleTime);

   float returnValue;
   if (currentCycleTime < halfSwivelTime)
   {
      // If we're in the first half of the swivel
      const float swivelHalfPortion = (currentCycleTime / halfSwivelTime);
      returnValue = swivelHalfPortion;
   }
   else if (currentCycleTime < halfSwivelTime + halfPauseTime)
   {
      // If we're paused at the far extreme
      returnValue = 1.0f;
   }
   else if (currentCycleTime < _swivelSeconds + halfPauseTime)
   {
      // If we're in the back half of the swivel, where we've already had one pause and one half-swivel
      const float swivelHalfPortion = ((currentCycleTime - halfPauseTime - halfSwivelTime) / halfSwivelTime);
      // N.B. Subtract from 1.0 since we're going in the opposite direction
      returnValue = 1.0f - swivelHalfPortion;
   }
   else
   {
      // If we're paused at the near extreme
      returnValue = 0;
   }
   return FMath::Clamp(returnValue, 0.f, 1.f);
}

void ATATVisionPerceptionDevice::_SetSwivelForTime(const float time)
{
   QUICK_SCOPE_CYCLE_COUNTER(TATVisionPerceptionDevice_Swivel);
   if (SwivelEnabled && SwivelPivotComponent)
   {
      _SetSwivelNormalizedProgress(_GetSwivelProgressForTime(time));
   }
}

bool ATATVisionPerceptionDevice::_ShouldPauseFromDetection() const
{
   return (_detectSuspiciousActorBehavior == ETATVisionPerceptionDeviceDetectSuspiciousActorBehavior::PauseOscillation)
      && _IsDetectingSuspiciousActor();
}

ETATVisionPerceptionDeviceMovementType ATATVisionPerceptionDevice::_GetRequiredMovementType() const
{
   if (_IsDetectingSuspiciousActor() &&
      _detectSuspiciousActorBehavior == ETATVisionPerceptionDeviceDetectSuspiciousActorBehavior::TrackFirstDetected)
   {
      return ETATVisionPerceptionDeviceMovementType::Track;
   }
   return ETATVisionPerceptionDeviceMovementType::Oscillate;
}

FVector2D ATATVisionPerceptionDevice::_GetSwivelOutputRangeForType() const
{
   switch(_swivelType)
   {
      case ETATVisionPerceptionDeviceRotationType::Swivel:
         return FVector2D(-_swivelAngle, _swivelAngle);
      case ETATVisionPerceptionDeviceRotationType::ThreeSixtyClockwise:
         return FVector2D(0, 360.f);
      case ETATVisionPerceptionDeviceRotationType::ThreeSixtyCounterClockwise:
         return FVector2D(360, 0);
   }
   return FVector2D(0, 0);
}

float ATATVisionPerceptionDevice::_GetRotationFromSceneComponent() const
{
   const FRotator currentComponentRotationMinusInitial = SwivelPivotComponent->GetRelativeRotation();
   float rotationValueForAxis = 0.f;
   switch(_swivelAxes)
   {
   case ETATVisionPerceptionDeviceSwivelAxes::Pitch:
      rotationValueForAxis = currentComponentRotationMinusInitial.Pitch;
      break;
   case ETATVisionPerceptionDeviceSwivelAxes::Yaw:
      rotationValueForAxis = currentComponentRotationMinusInitial.Yaw;
      break;
   default:
      checkNoEntry();
   }
   return rotationValueForAxis = FMath::UnwindDegrees(rotationValueForAxis);
}

void ATATVisionPerceptionDevice::_AuthoritySyncOscillationState()
{
   check(HasAuthority());

   const ETATVisionPerceptionDeviceMovementType newIsOscillatingType = _GetRequiredMovementType();
   if (_oscillationState.MovementType != newIsOscillatingType)
   {
      FlushNetDormancy();
      const float currentWorldTimeSeconds = GetWorld()->GetTimeSeconds();
      switch(newIsOscillatingType)
      {
         case ETATVisionPerceptionDeviceMovementType::Oscillate:
         {
            if(_oscillationState.MovementType == ETATVisionPerceptionDeviceMovementType::Track)
            {
               _oscillationState.ServerTimeWhenFullyReset = currentWorldTimeSeconds + _swivelResetTime + _swivelPauseSeconds;
            }
            _oscillationState.MovementType = ETATVisionPerceptionDeviceMovementType::Oscillate;
            break;
         }
         case ETATVisionPerceptionDeviceMovementType::Track:
         {
            _oscillationState.MovementType = ETATVisionPerceptionDeviceMovementType::Track;
            _oscillationState.ServerTimeWhenFullyTracksActor = currentWorldTimeSeconds + _timeToFullyRotateCameraToLookAtTarget;

            if(const FTrackedActor* trackedActorData = _trackedActors.FindByPredicate([&](const FTrackedActor& trackedActor)
            {
               return trackedActor.status == ETrackedActorStatus::Enemy;
            }))
            {
               _oscillationState.TrackedActor = trackedActorData->actor.Get();
            }

            break;
         }
      }
   }
}

void ATATVisionPerceptionDevice::_AuthorityTickDeviceState()
{
   check(HasAuthority());

   // Remove any actors we think we see that are no longer around
   _FilterOutDeadSeenActors();

   // Check status of each actor we're tracking, and see if any have changed
   _UpdateTrackedActorStatuses();
   
   // suppressed
   if (_deviceState == ETATVisionPerceptionDeviceState::Suppressed ||
       _deviceState == ETATVisionPerceptionDeviceState::Destroyed)
      return;


   // if we see suspicious actors we're either warning or triggered depending on the elapsed time
   if (_IsDetectingSuspiciousActor())
   {
      ensure(_suspiciousActorsSeenWorldTimeStart != float(INDEX_NONE));

      const float now = GetWorld()->GetTimeSeconds();
      const float timeSinceEnemyFirstSeen = now - _suspiciousActorsSeenWorldTimeStart;
      if (timeSinceEnemyFirstSeen > _secondsUntilAlarmTriggers)
      {
         _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState::Triggered);

         // and try to trigger any alarms that are ready to be triggered
         _AuthorityTryTriggerAlarms();
      }
      else
      {
         _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState::Warning);
      }
   }
   else if (_shouldReactToFriendlies && _IsDetectingFriendlyActors())
   {
      _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState::DetectedFriendly);
   }
   else
   {
      const float now = GetWorld()->GetTimeSeconds();
      if (
         (_deviceState == ETATVisionPerceptionDeviceState::Triggered || _deviceState == ETATVisionPerceptionDeviceState::Warning)
         && _deviceState != ETATVisionPerceptionDeviceState::PausedAfterTrigger
         )
      {
         _timeUntilPauseStateShouldReset = now + _timeUntilPauseStateExitAfterTrigger;
         _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState::PausedAfterTrigger);
      }
      else if(_timeUntilPauseStateShouldReset < now && _deviceState != ETATVisionPerceptionDeviceState::Normal)
      {
         _AuthoritySetDeviceState(ETATVisionPerceptionDeviceState::Normal);
      }
   }
}

void ATATVisionPerceptionDevice::_AuthorityTryTriggerAlarms()
{
   if(_bShouldTriggerAlarm == false)
      return;
   check(HasAuthority());

   bool alarmWasTriggered = false;
   for (ATATSquadAlarmStation* alarmStation : AlarmStations)
   {
      if (!IsValid(alarmStation))
         continue;

      if (alarmStation->GetState() == EAlarmState::Ready)
      {
         alarmStation->AuthorityTrigger();
         alarmWasTriggered = true;
      }
   }

   if (alarmWasTriggered)
   {
      _OnDeviceTriggeredAlarm();
   }
}

void ATATVisionPerceptionDevice::_AuthorityTargetSightPerceptionUpdated(AActor* actor, FAIStimulus stimulus)
{
   check(HasAuthority());

   if (stimulus.WasSuccessfullySensed())
   {
      bool alreadyPresent = _trackedActors.ContainsByPredicate([&](const FTrackedActor& trackedActor)
      {
         return trackedActor.actor == actor;
      });

      if (alreadyPresent == false)
      {
         // N.B. Even if `_shouldReactToFriendlies` is false, we still need to track friendlies that enter our perception,
         // because it's possible for their team to change to Hostilehostile while in our vision: in that case, we'd want to know about them so we can react
         // when that happens
         FTrackedActor newlyTrackedActor;
         newlyTrackedActor.actor = actor;
         newlyTrackedActor.status = _GetStatusOfTrackedActor(actor);
         _trackedActors.Add(newlyTrackedActor);
      }
   }
   else
   {
      // we stopped tracking the actor that we're currently tracking, unset it
      if(actor == _oscillationState.TrackedActor)
      {
         _oscillationState.TrackedActor = nullptr;
      }
      _trackedActors.RemoveAll([&](const FTrackedActor& trackedActor)
      {
         return trackedActor.actor == actor;
      });
   }

   _CheckIfSuspiciousActorsSeenChanged();
}

void ATATVisionPerceptionDevice::_AuthorityOnAlarmSystemStateChanged(EAlarmState newState, EAlarmState previousState, bool isRecent)
{
   check(HasAuthority());

   // the alarm system state has changed, but we still see a player, so maybe re-trigger the alarm
   if (_deviceState == ETATVisionPerceptionDeviceState::Triggered)
   {
      _AuthorityTryTriggerAlarms();
   }
}

void ATATVisionPerceptionDevice::_OnRep_DeviceState(ETATVisionPerceptionDeviceState oldDeviceState)
{
   _BroadcastDeviceStateChanged(oldDeviceState);
}

void ATATVisionPerceptionDevice::_BroadcastDeviceStateChanged(ETATVisionPerceptionDeviceState oldDeviceState)
{
   // internal
   _OnDeviceStateChanged(_deviceState, oldDeviceState);

   // external
   OnDeviceStateChanged.Broadcast(_deviceState, oldDeviceState);
}

void ATATVisionPerceptionDevice::_AuthoritySetDeviceState(ETATVisionPerceptionDeviceState deviceState)
{
   check(HasAuthority());

   if (_deviceState != deviceState)
   {
      FlushNetDormancy();
      ETATVisionPerceptionDeviceState oldDeviceState = _deviceState;
      _deviceState = deviceState;
      _BroadcastDeviceStateChanged(oldDeviceState);
   }
}

void ATATVisionPerceptionDevice::_CheckIfSuspiciousActorsSeenChanged()
{
   if (_IsDetectingSuspiciousActor())
   {
      if (_suspiciousActorsSeenWorldTimeStart == float(INDEX_NONE))
      {
         _suspiciousActorsSeenWorldTimeStart = GetWorld()->GetTimeSeconds();
      }
   }
   else
   {
      _suspiciousActorsSeenWorldTimeStart = float(INDEX_NONE);
   }

   _AuthoritySyncOscillationState();
}

void ATATVisionPerceptionDevice::_FilterOutDeadSeenActors()
{
   int32 numActorsRemoved = _trackedActors.RemoveAll([](const FTrackedActor& trackedActor)
   {
      return !trackedActor.actor.IsValid();
   });

   // If an actor in our vision died, we may need to unpause our oscillation, and if it was
   // the only suspicious actor in our sight, we need to reset _enemiesSeenWorldTimeStart
   if (numActorsRemoved > 0)
   {
      _CheckIfSuspiciousActorsSeenChanged();
   }
}

void ATATVisionPerceptionDevice::_UpdateTrackedActorStatuses()
{
   bool anyChanges = false;

   _bIsDetectingEnemyActor = false;
   _bIsDetectingFriendlyActor = false;
   _bIsDetectingSuspiciousActor = false;

   for (FTrackedActor& trackedActor : _trackedActors)
   {
      if (AActor* actor = trackedActor.actor.Get())
      {
         const ETrackedActorStatus newStatus = _GetStatusOfTrackedActor(actor);
         if (newStatus != trackedActor.status)
         {
            anyChanges = true;
            trackedActor.status = newStatus;
         }
         switch(trackedActor.status)
         {
            case ETrackedActorStatus::Enemy:
               _bIsDetectingEnemyActor = true;
            case ETrackedActorStatus::UnconsciousFriendly:
               _bIsDetectingSuspiciousActor = true;
               break;
            case ETrackedActorStatus::ConsciousFriendly:
               _bIsDetectingFriendlyActor = true;
               break;
            default:
               break;
         }
      }
   }

   // If there were any changes, we may need to re-evaluate if the camera should be paused or not
   if (anyChanges)
   {
      // Remove any actors that we don't care about
      _trackedActors.RemoveAll([](const FTrackedActor& trackedActor)
      {
         return trackedActor.status == ETrackedActorStatus::Untracked;
      });
   }
   
   _CheckIfSuspiciousActorsSeenChanged();
}

ATATVisionPerceptionDevice::ETrackedActorStatus ATATVisionPerceptionDevice::_GetStatusOfTrackedActor(AActor* actor) const
{
   const EOSETeamAttitude attitudeToActor = _GetAttitudeTowardsActor(actor);
   if (attitudeToActor == EOSETeamAttitude::Hostile)
   {
      bool isUnconscious = false;
      if (const AOSECharacterBase* character = Cast<AOSECharacterBase>(actor))
      {
         isUnconscious = character->IsUnconscious();
      }
      if(isUnconscious == false)
      {
         return ETrackedActorStatus::Enemy;
      }
   }
   else if (attitudeToActor == EOSETeamAttitude::Friendly)
   {
      if (const AOSECharacterBase* character = Cast<AOSECharacterBase>(actor))
      {
         if (character->IsLyingDown())
         {
            return ETrackedActorStatus::UnconsciousFriendly;
         }
      }

      return ETrackedActorStatus::ConsciousFriendly;
   }
   return ETrackedActorStatus::Untracked;
}

uint8 ATATVisionPerceptionDevice::GetTeam() const
{
   return UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Guard);
}

EOSETeamAttitude ATATVisionPerceptionDevice::_GetAttitudeTowardsActor(const AActor* actor) const
{
   if (!actor->Implements<UOSETeamInterface>())
   {
      return EOSETeamAttitude::Neutral;
   }
   if(const IGameplayTagAssetInterface* tagAssetInterface = Cast<IGameplayTagAssetInterface>(actor))
   {
      if(tagAssetInterface->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING))
      {
         return EOSETeamAttitude::Hostile;
      }
   }
   return UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(this, actor, DisguiseHandling);
}

