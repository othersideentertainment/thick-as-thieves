// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionOSEMoveToForce.h"

// ose

// ue4
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "DrawDebugHelpers.h"
#include "Curves/CurveVector.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_ApplyRootMotionOSEMoveToForce)

//---------------------------------------------------------------------------
// FRootMotionSource_OSEMoveToForce
//---------------------------------------------------------------------------

FRootMotionSource_OSEMoveToForce::FRootMotionSource_OSEMoveToForce()
   : StartLocation(ForceInitToZero)
   , TargetLocation(ForceInitToZero)
   , RestrictSpeedToExpected(false)
   , PathOffsetCurve(nullptr)
   , MotionLerpCurve(nullptr)
{
}

FRootMotionSource* FRootMotionSource_OSEMoveToForce::Clone() const
{
   FRootMotionSource_OSEMoveToForce* copyPtr = new FRootMotionSource_OSEMoveToForce(*this);
   return copyPtr;
}

bool FRootMotionSource_OSEMoveToForce::Matches(const FRootMotionSource* other) const
{
   if (!FRootMotionSource::Matches(other))
   {
      return false;
   }

   // We can cast safely here since in FRootMotionSource::Matches() we ensured ScriptStruct equality
   const FRootMotionSource_OSEMoveToForce* otherCast = static_cast<const FRootMotionSource_OSEMoveToForce*>(other);

   return RestrictSpeedToExpected == otherCast->RestrictSpeedToExpected &&
          PathOffsetCurve == otherCast->PathOffsetCurve &&
          MotionLerpCurve == otherCast->MotionLerpCurve &&
          FVector::PointsAreNear(TargetLocation, otherCast->TargetLocation, 0.1f);
}

bool FRootMotionSource_OSEMoveToForce::MatchesAndHasSameState(const FRootMotionSource* other) const
{
   // Check that it matches
   if (!FRootMotionSource::MatchesAndHasSameState(other))
   {
      return false;
   }

   return true; // OSEMoveToForce has no unique state
}

bool FRootMotionSource_OSEMoveToForce::UpdateStateFrom(const FRootMotionSource* sourceToTakeStateFrom, bool markForSimulatedCatchup)
{
   if (!FRootMotionSource::UpdateStateFrom(sourceToTakeStateFrom, markForSimulatedCatchup))
   {
      return false;
   }

   return true; // OSEMoveToForce has no unique state other than Time which is handled by FRootMotionSource
}

void FRootMotionSource_OSEMoveToForce::SetTime(float newTime)
{
   FRootMotionSource::SetTime(newTime);

   // TODO-RootMotionSource: Check if reached destination?
}

FVector FRootMotionSource_OSEMoveToForce::GetPathOffsetInWorldSpace(float moveFraction) const
{
   if (PathOffsetCurve)
   {
      // Calculate path offset
      const FVector pathOffsetInFacingSpace = PathOffsetCurve->GetVectorValue(moveFraction);
      FRotator facingRotation((TargetLocation - StartLocation).Rotation());
      facingRotation.Pitch = 0.f; // By default we don't include pitch in the offset, but an option could be added if necessary
      return facingRotation.RotateVector(pathOffsetInFacingSpace);
   }

   return FVector::ZeroVector;
}

void FRootMotionSource_OSEMoveToForce::PrepareRootMotion(
   float simulationTime,
   float movementTickTime,
   const ACharacter& character,
   const UCharacterMovementComponent& moveComponent
)
{
   RootMotionParams.Clear();

   if (Duration > SMALL_NUMBER && movementTickTime > SMALL_NUMBER)
   {
      const float fixedMoveFraction = (GetTime() + simulationTime) / Duration;

      // sample the motion curve to figure out where we should be along the path
      float moveFraction = fixedMoveFraction;
      if (MotionLerpCurve)
      {
         moveFraction = FMath::Clamp(MotionLerpCurve->GetFloatValue(moveFraction), 0.0f, 1.0f);
      }

      FVector currentTargetLocation = FMath::Lerp<FVector, float>(StartLocation, TargetLocation, moveFraction);
      currentTargetLocation += GetPathOffsetInWorldSpace(moveFraction);

      const FVector currentLocation = character.GetActorLocation();

      FVector forceLocation = (currentTargetLocation - currentLocation) / movementTickTime;

      if (RestrictSpeedToExpected && !forceLocation.IsNearlyZero(KINDA_SMALL_NUMBER))
      {
         // Calculate expected current location (if we didn't have collision and moved exactly where our velocity should have taken us)
         const float previousMoveFraction = GetTime() / Duration;
         FVector currentExpectedLocation = FMath::Lerp<FVector, float>(StartLocation, TargetLocation, previousMoveFraction);
         currentExpectedLocation += GetPathOffsetInWorldSpace(previousMoveFraction);

         // Restrict speed to the expected speed, allowing some small amount of error
         const FVector expectedForce = (currentTargetLocation - currentExpectedLocation) / movementTickTime;
         const float expectedSpeed = expectedForce.Size();
         const float currentSpeedSqr = forceLocation.SizeSquared();

         const float kErrorAllowance = 0.5f; // in cm/s
         if (currentSpeedSqr > FMath::Square(expectedSpeed + kErrorAllowance))
         {
            forceLocation.Normalize();
            forceLocation *= expectedSpeed;
         }
      }

      // Debug
#if ROOT_MOTION_DEBUG
      if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() != 0)
      {
         const FVector locDiff = moveComponent.UpdatedComponent->GetComponentLocation() - currentLocation;
         static const float kDebugLifetime = 6.0f;

         // Current
         DrawDebugCapsule(character.GetWorld(), moveComponent.UpdatedComponent->GetComponentLocation(), character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::Red, true, kDebugLifetime);

         // Current Target
         DrawDebugCapsule(character.GetWorld(), currentTargetLocation + locDiff, character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::Green, true, kDebugLifetime);

         // Target
         DrawDebugCapsule(character.GetWorld(), TargetLocation + locDiff, character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::Blue, true, kDebugLifetime);

         // Force
         DrawDebugLine(character.GetWorld(), currentLocation, currentLocation + forceLocation, FColor::Blue, true, kDebugLifetime);
      }
#endif

      FTransform newTransform(forceLocation);
      RootMotionParams.Set(newTransform);
   }
   else
   {
      checkf(Duration > SMALL_NUMBER, TEXT("FRootMotionSource_OSEMoveToForce prepared with invalid duration."));
   }

   SetTime(GetTime() + simulationTime);
}

bool FRootMotionSource_OSEMoveToForce::NetSerialize(FArchive& ar, UPackageMap* map, bool& outSuccess)
{
   if (!FRootMotionSource::NetSerialize(ar, map, outSuccess))
   {
      return false;
   }

   ar << StartLocation; // TODO-RootMotionSource: Quantization
   ar << TargetLocation; // TODO-RootMotionSource: Quantization
   ar << RestrictSpeedToExpected;
   ar << PathOffsetCurve;
   ar << MotionLerpCurve;

   outSuccess = true;
   return true;
}

UScriptStruct* FRootMotionSource_OSEMoveToForce::GetScriptStruct() const
{
   return FRootMotionSource_OSEMoveToForce::StaticStruct();
}

FString FRootMotionSource_OSEMoveToForce::ToSimpleString() const
{
   return FString::Printf(TEXT("[ID:%u]FRootMotionSource_OSEMoveToForce %s"), LocalID, *InstanceName.GetPlainNameString());
}

void FRootMotionSource_OSEMoveToForce::AddReferencedObjects(class FReferenceCollector& collector)
{
   collector.AddReferencedObject(PathOffsetCurve);
   collector.AddReferencedObject(MotionLerpCurve);

   FRootMotionSource::AddReferencedObjects(collector);
}

//---------------------------------------------------------------------------
// UAbilityTask_ApplyRootMotionMoveToTransformForce
//---------------------------------------------------------------------------

UAbilityTask_ApplyRootMotionOSEMoveToForce::UAbilityTask_ApplyRootMotionOSEMoveToForce(const FObjectInitializer& objectInitializer)
: Super(objectInitializer)
{
   _setNewMovementMode = false;
   _newMovementMode = EMovementMode::MOVE_Walking;
   _previousMovementMode = EMovementMode::MOVE_None;
   _restrictSpeedToExpected = false;
   _pathOffsetCurve = nullptr;
}

UAbilityTask_ApplyRootMotionOSEMoveToForce* UAbilityTask_ApplyRootMotionOSEMoveToForce::ApplyRootMotionOSEMoveToForce(UGameplayAbility* owningAbility, FName taskInstanceName, FVector targetLocation, float duration, bool setNewMovementMode, EMovementMode movementMode, bool restrictSpeedToExpected, UCurveVector* pathOffsetCurve, UCurveFloat* motionLerpCurve, ERootMotionFinishVelocityMode velocityOnFinishMode, FVector setVelocityOnFinish, float clampVelocityOnFinish)
{
   UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Duration(duration);

   UAbilityTask_ApplyRootMotionOSEMoveToForce* myTask = NewAbilityTask<UAbilityTask_ApplyRootMotionOSEMoveToForce>(owningAbility, taskInstanceName);

   myTask->ForceName = taskInstanceName;
   myTask->_targetLocation = targetLocation;
   myTask->_duration = FMath::Max(duration, KINDA_SMALL_NUMBER); // Avoid negative or divide-by-zero cases
   myTask->_setNewMovementMode = setNewMovementMode;
   myTask->_newMovementMode = movementMode;
   myTask->_restrictSpeedToExpected = restrictSpeedToExpected;
   myTask->_pathOffsetCurve = pathOffsetCurve;
   myTask->_motionLerpCurve = motionLerpCurve;
   myTask->FinishVelocityMode = velocityOnFinishMode;
   myTask->FinishSetVelocity = setVelocityOnFinish;
   myTask->FinishClampVelocity = clampVelocityOnFinish;
   if (AActor* avatarActor = myTask->GetAvatarActor())
   {
      myTask->_startLocation = avatarActor->GetActorLocation();
   }
   else
   {
      checkf(false, TEXT("UAbilityTask_ApplyRootMotionOSEMoveToForce called without valid avatar actor to get start location from."));
      myTask->_startLocation = targetLocation;
   }
   myTask->SharedInitAndApply();

   return myTask;
}

void UAbilityTask_ApplyRootMotionOSEMoveToForce::SharedInitAndApply()
{
   if (AbilitySystemComponent->AbilityActorInfo->MovementComponent.IsValid())
   {
      MovementComponent = Cast<UCharacterMovementComponent>(AbilitySystemComponent->AbilityActorInfo->MovementComponent.Get());
      StartTime = GetWorld()->GetTimeSeconds();
      EndTime = StartTime + _duration;

      if (MovementComponent)
      {
         if (_setNewMovementMode)
         {
            _previousMovementMode = MovementComponent->MovementMode;
            MovementComponent->SetMovementMode(_newMovementMode);
         }

         ForceName = ForceName.IsNone() ? FName("AbilityTaskApplyRootMotionOSEMoveToForce") : ForceName;
         TSharedPtr<FRootMotionSource_OSEMoveToForce> moveToForce = MakeShared<FRootMotionSource_OSEMoveToForce>();
         moveToForce->InstanceName = ForceName;
         moveToForce->AccumulateMode = ERootMotionAccumulateMode::Override;
         moveToForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
         moveToForce->Priority = 1000;
         moveToForce->StartLocation = _startLocation;
         moveToForce->TargetLocation = _targetLocation;
         moveToForce->Duration = _duration;
         moveToForce->RestrictSpeedToExpected = _restrictSpeedToExpected;
         moveToForce->PathOffsetCurve = _pathOffsetCurve;
         moveToForce->MotionLerpCurve = _motionLerpCurve;
         moveToForce->FinishVelocityParams.Mode = FinishVelocityMode;
         moveToForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
         moveToForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
         RootMotionSourceID = MovementComponent->ApplyRootMotionSource(moveToForce);
      }
   }
   else
   {
      ABILITY_LOG(Warning, TEXT("UAbilityTask_ApplyRootMotionOSEMoveToForce called in Ability %s with null MovementComponent; Task Instance Name %s."), 
         Ability ? *Ability->GetName() : TEXT("NULL"), 
         *InstanceName.ToString());
   }
}

void UAbilityTask_ApplyRootMotionOSEMoveToForce::TickTask(float deltaTime)
{
   if (bIsFinished)
   {
      return;
   }

   Super::TickTask(deltaTime);

   AActor* myActor = GetAvatarActor();
   if (myActor)
   {
      const bool timedOut = HasTimedOut();
      const float reachedDestinationDistanceSqr = 50.f * 50.f;
      FVector actorLocation = myActor->GetActorLocation();
      float distSq = FVector::DistSquared(_targetLocation, actorLocation);
      const bool reachedDestination = distSq < reachedDestinationDistanceSqr;

      if (timedOut)
      {
         // Task has finished
         bIsFinished = true;

         if (!bIsSimulating)
         {
            myActor->ForceNetUpdate();
            if (reachedDestination)
            {
               if (ShouldBroadcastAbilityTaskDelegates())
               {
                  OnTimedOutAndDestinationReached.Broadcast();
               }
            }
            else
            {
               if (ShouldBroadcastAbilityTaskDelegates())
               {
                  OnTimedOut.Broadcast();
               }
            }
            EndTask();
         }
      }
   }
   else
   {
      bIsFinished = true;
      EndTask();
   }
}

void UAbilityTask_ApplyRootMotionOSEMoveToForce::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _startLocation);
   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _targetLocation);
   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _duration);
   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _setNewMovementMode);
   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _newMovementMode);
   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _restrictSpeedToExpected);
   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _pathOffsetCurve);
   DOREPLIFETIME(UAbilityTask_ApplyRootMotionOSEMoveToForce, _motionLerpCurve);   
}

void UAbilityTask_ApplyRootMotionOSEMoveToForce::PreDestroyFromReplication()
{
   bIsFinished = true;
   EndTask();
}

void UAbilityTask_ApplyRootMotionOSEMoveToForce::OnDestroy(bool abilityIsEnding)
{
   if (MovementComponent)
   {
      MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);

      if (_setNewMovementMode)
      {
         MovementComponent->SetMovementMode(_previousMovementMode);
      }
   }

   Super::OnDestroy(abilityIsEnding);
}

