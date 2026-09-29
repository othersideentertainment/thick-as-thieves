// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Abilities/Tasks/AbilityTask.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotion_Base.h"
#include "GameFramework/RootMotionSource.h"
#include "UObject/ObjectMacros.h"
#include "AbilityTask_ApplyRootMotionMoveToTransformForce.generated.h"

class UCharacterMovementComponent;
class UCurveVector;
class UGameplayTasksComponent;
enum class ERootMotionFinishVelocityMode : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FApplyRootMotionMoveToTransformForceDelegate);

class AActor;

//-------------------------------------------------------------------------------------------------------------------------------
// FRootMotionSource_MoveToTransformForce - moves the target to a given fixed location/rotation in world space over the duration
//-------------------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct OSECORE_API FRootMotionSource_MoveToTransformForce : public FRootMotionSource
{
   GENERATED_USTRUCT_BODY()

   FRootMotionSource_MoveToTransformForce();

   virtual ~FRootMotionSource_MoveToTransformForce() {}

   UPROPERTY()
   FVector StartLocation;

   UPROPERTY()
   FQuat StartRotation;

   UPROPERTY()
   FVector TargetLocation;

   UPROPERTY()
   FQuat TargetRotation;

   UPROPERTY()
   bool RestrictSpeedToExpected;

   UPROPERTY()
   TObjectPtr<UCurveVector> PathOffsetCurve = nullptr;

   UPROPERTY()
   TObjectPtr<UCurveFloat> MotionLerpCurve = nullptr;

   FVector GetPathOffsetInWorldSpace(float moveFraction) const;
   virtual FRootMotionSource* Clone() const override;
   virtual bool Matches(const FRootMotionSource* other) const override;
   virtual bool MatchesAndHasSameState(const FRootMotionSource* other) const override;
   virtual bool UpdateStateFrom(const FRootMotionSource* sourceToTakeStateFrom, bool markForSimulatedCatchup = false) override;
   virtual void SetTime(float newTime) override;
   virtual void PrepareRootMotion(
      float simulationTime, 
      float movementTickTime,
      const ACharacter& character, 
      const UCharacterMovementComponent& moveComponent) override;
   virtual bool NetSerialize(FArchive& ar, UPackageMap* map, bool& outSuccess) override;
   virtual UScriptStruct* GetScriptStruct() const override;
   virtual FString ToSimpleString() const override;
   virtual void AddReferencedObjects(class FReferenceCollector& collector) override;
};

template<>
struct TStructOpsTypeTraits<FRootMotionSource_MoveToTransformForce> : public TStructOpsTypeTraitsBase2<FRootMotionSource_MoveToForce>
{
   enum
   {
      WithNetSerializer = true,
      WithCopy = true
   };
};

//-------------------------------------------------------------------------------------------------------------------------------
// UAbilityTask_ApplyRootMotionMoveToTransformForce -  Applies force to character's movement
//-------------------------------------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API UAbilityTask_ApplyRootMotionMoveToTransformForce : public UAbilityTask_ApplyRootMotion_Base
{
   GENERATED_UCLASS_BODY()

   UPROPERTY(BlueprintAssignable)
   FApplyRootMotionMoveToTransformForceDelegate OnTimedOut;

   UPROPERTY(BlueprintAssignable)
   FApplyRootMotionMoveToTransformForceDelegate OnTimedOutAndDestinationReached;

   /** Apply force to character's movement */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_ApplyRootMotionMoveToTransformForce* ApplyRootMotionMoveToTransformForce(
      UGameplayAbility* owningAbility,
      FName taskInstanceName,
      FVector targetLocation,
      FRotator targetRotation,
      float duration,
      bool setNewMovementMode,
      EMovementMode movementMode,
      bool restrictSpeedToExpected,
      UCurveVector* pathOffsetCurve,
      UCurveFloat* motionLerpCurve,
      ERootMotionFinishVelocityMode velocityOnFinishMode,
      FVector setVelocityOnFinish,
      float clampVelocityOnFinish,
      bool teleportToTransformOnFinish,
      bool sweepTeleportOnFinish = true);

   /** Tick function for this task, if bTickingTask == true */
   virtual void TickTask(float deltaTime) override;

   virtual void PreDestroyFromReplication() override;
   virtual void OnDestroy(bool abilityIsEnding) override;

protected:

   virtual void SharedInitAndApply() override;

protected:

   UPROPERTY(Replicated)
   FVector _startLocation;

   UPROPERTY(Replicated)
   FQuat _startRotation;

   UPROPERTY(Replicated)
   FVector _targetLocation;

   UPROPERTY(Replicated)
   FQuat _targetRotation;

   UPROPERTY(Replicated)
   float _duration;

   UPROPERTY(Replicated)
   bool _setNewMovementMode;

   UPROPERTY(Replicated)
   TEnumAsByte<EMovementMode> _newMovementMode;

   /** If enabled, we limit velocity to the initial expected velocity to go distance to the target over Duration.
    *  This prevents cases of getting really high velocity the last few frames of the root motion if you were being blocked by
    *  collision. Disabled means we do everything we can to velocity during the move to get to the TargetLocation. */
   UPROPERTY(Replicated)
   bool _restrictSpeedToExpected;

   UPROPERTY(Replicated)
   bool _teleportToTransformOnFinish;
   
   UPROPERTY(Replicated)
   bool _shouldSweepTeleport { false };

   UPROPERTY(Replicated)
   UCurveVector* _pathOffsetCurve = nullptr;

   UPROPERTY(Replicated)
   UCurveFloat* _motionLerpCurve = nullptr;

   EMovementMode _previousMovementMode;
};

