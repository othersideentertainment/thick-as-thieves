// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Patrol/PatrolPath.h"

// ue5
#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"

#include "TATFlyingPatrolFollowingMovementComponent.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TAT_API UTATFlyingPatrolFollowingMovementComponent : public UMovementComponent
{
   GENERATED_BODY()

public:
   UTATFlyingPatrolFollowingMovementComponent();

   UPROPERTY(EditInstanceOnly, Category="TAT|Patrol Follower")
   APatrolPath* PatrolPath { nullptr };

   // from TATInterpolatedMovementComponent
   void MoveInterpolationTarget(const FVector& newLocation, const FRotator& newRotation);
   void ResetInterpolation();
   void SetInterpolatedComponent(USceneComponent* component);
   // end TATInterpolatedMovementComponent

protected:
   UPROPERTY(EditDefaultsOnly, Category="TAT|Patrol Follower")
   float _timeToPauseForWhenImpactingObject { 1.f };

   // The speed in m/s this actor should move
   UPROPERTY(EditDefaultsOnly, Category="TAT|Patrol Follower")
   float _movementSpeed { 1.f };

   // The speed in degrees per second the actor should move
   UPROPERTY(EditDefaultsOnly, Category="TAT|Patrol Follower")
   float _rotationSpeed { 1.f };

   // Should the actor rotate towards the direction of travel?
   UPROPERTY(EditDefaultsOnly, Category="TAT|Patrol Follower")
   bool _orientToDirectionOfTravel { true };

   // The distance is cm that the actor should get from a patrol point before moving to the next 
   UPROPERTY(EditDefaultsOnly, Category="TAT|Patrol Follower")
   float _distanceFromPatrolPointUntilMoveToNext;
   
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   // In order to avoid snapping to the actual location when we mis-predict, we interpolate the visuals to the target
   // location over a defined amount of time.
   void _TickInterpolation(float deltaTime);

   // This should be the component for the visuals, ideally below the collider component
   USceneComponent* _GetInterpolatedComponent() const;
   
   UPROPERTY(Replicated)
   int _patrolIndex {0};
   float _waitTime {0.f};

   UPROPERTY(Replicated)
   float _currentPauseTime {0.f};

   // from TATInterpolatedMovementComponent - perhaps this should be in a helper function shared between both classes
   // it's very close to the projectile movement logic too.
   UPROPERTY(EditDefaultsOnly, Category = Interpolation)
   bool _interpolationMovement;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation)
   bool _interpolationRotation;

   bool _interpolationComplete;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpolationLocationTime;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpolationRotationTime;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpolationLocationMaxLagDistance;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpolationLocationSnapToTargetDistance;
   
   TWeakObjectPtr<USceneComponent> _interpolatedComponentPtr;
   FVector _interpolationLocationOffset;
   FVector _interpolationInitialLocationOffset;
   FQuat _interpolationRotationOffset;
   FQuat _interpolationInitialRotationOffset;
};
