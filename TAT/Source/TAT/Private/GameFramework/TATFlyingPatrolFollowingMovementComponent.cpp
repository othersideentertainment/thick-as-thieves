// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATFlyingPatrolFollowingMovementComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFlyingPatrolFollowingMovementComponent)
namespace TATFlyingPatrolFollowingMovementComponentCVars
{
   static int32 RequestMispredict = 0;
   static FAutoConsoleVariableRef CVarRequestMispredict(TEXT("TAT.FlyingPatrol.RequestMispredict"),
   RequestMispredict, TEXT("Force misprediction"), ECVF_Default);
   static int32 VisualizeServerAndClientLocation = 0;
   static FAutoConsoleVariableRef CVarVisualizeServerAndClientLocation(TEXT("TAT.FlyingPatrol.VisualizeServerAndClientLocation"),
      VisualizeServerAndClientLocation, TEXT("Show a yellow sphere for the server position and a green sphere for the simulated proxy position"), ECVF_Default);
}

UTATFlyingPatrolFollowingMovementComponent::UTATFlyingPatrolFollowingMovementComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   SetIsReplicatedByDefault(true);
}

void UTATFlyingPatrolFollowingMovementComponent::TickComponent(float deltaTime, ELevelTick tickType,
                                                               FActorComponentTickFunction* thisTickFunction)
{
   const AActor* owner = GetOwner();
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
   
   const UWorld* world = GetWorld();
   if(world == nullptr)
      return;
   
   const AGameStateBase* gameStateBase = world->GetGameState();
   
   if(gameStateBase == nullptr)
      // while the game state base is replicating to the client, we're going to drift out of sync
      return;
   
   const float currentTime = gameStateBase->GetServerWorldTimeSeconds();
#if ENABLE_DRAW_DEBUG
   if(TATFlyingPatrolFollowingMovementComponentCVars::VisualizeServerAndClientLocation != 0)
   {
      DrawDebugSphere(world, UpdatedComponent->GetComponentLocation(), 32, 16, GetOwnerRole() == ROLE_Authority ? FColor::Yellow : FColor::Green);
   }
#endif
   
   if(_currentPauseTime > currentTime)
   {
      // After hitting an object, we pause movement for a fixed amount of time
      return;
   }
   
   if(PatrolPath == nullptr)
   {
      return;
   }

   if(_patrolIndex >= PatrolPath->GetNumPoints())
   {
      return;
   }
   
   if (_interpolationMovement && !_interpolationComplete)
   {
      _TickInterpolation(deltaTime);
   }
   
   const FVector currentPosition = UpdatedComponent->GetComponentLocation();
   const FVector targetPosition = PatrolPath->GetPointLocationWorldSpace(_patrolIndex);
   const FVector direction = (targetPosition - currentPosition).GetSafeNormal();
   
   const float distToEndSq = FVector::DistSquared(currentPosition, targetPosition);

   float speedToUse = _movementSpeed * deltaTime;

   // slow down if getting close to the position so we don't overshoot
   if (distToEndSq < FMath::Square(_movementSpeed))
   {
      speedToUse *= FMath::Clamp(FMath::Sqrt(distToEndSq) / _movementSpeed, 0.0f, 1.0f);
   }
   
   if(TATFlyingPatrolFollowingMovementComponentCVars::RequestMispredict != 0)
   {
      _currentPauseTime = currentTime + _timeToPauseForWhenImpactingObject;
      TATFlyingPatrolFollowingMovementComponentCVars::RequestMispredict = 0;
   }
   
   const FVector delta = direction * speedToUse;
   const FRotator rotationDirection = _orientToDirectionOfTravel ? delta.Rotation() : UpdatedComponent->GetComponentRotation();
   const FRotator calculatedRotation = FMath::RInterpTo(UpdatedComponent->GetComponentRotation(), rotationDirection, deltaTime, _rotationSpeed);

   FHitResult hit(1.f);
   SafeMoveUpdatedComponent(delta, calculatedRotation, true, hit, ETeleportType::None);
   if(hit.IsValidBlockingHit())
   {
      HandleImpact(hit, deltaTime, delta);
      if(GetOwner()->HasAuthority())
      {
         _currentPauseTime = currentTime + _timeToPauseForWhenImpactingObject;
      }
   }

   // Movement can cause the destruction of the component (a trigger that instantly destroys)
   if (!IsValid(owner) || !UpdatedComponent || !IsActive())
   {
      return;
   }
   
   if(FVector::Distance(targetPosition, UpdatedComponent->GetComponentLocation()) < _distanceFromPatrolPointUntilMoveToNext)
   {
      _patrolIndex++;
      if(_patrolIndex >=  PatrolPath->GetNumPoints())
      {
         _patrolIndex = 0;
      }
   }
   UpdateComponentVelocity();
}

void UTATFlyingPatrolFollowingMovementComponent::_TickInterpolation(float deltaTime)
{
   if (!_interpolationComplete)
   {
      if (USceneComponent* interpComponent = _GetInterpolatedComponent())
      {
         // Smooth location. Interp faster when stopping.
         const float actualInterpLocationTime = Velocity.IsZero() ? 0.5f * _interpolationLocationTime : _interpolationLocationTime;
         if (deltaTime < actualInterpLocationTime)
         {
            // Slowly decay translation offset (lagged exponential smoothing)
            _interpolationLocationOffset = (_interpolationLocationOffset * (1.f - deltaTime / actualInterpLocationTime));
         }
         else
         {
            _interpolationLocationOffset = FVector::ZeroVector;
         }

         // Smooth rotation
         if (deltaTime < _interpolationRotationTime && _interpolationRotation)
         {
            // Slowly decay rotation offset
            _interpolationRotationOffset = FQuat::FastLerp(_interpolationRotationOffset, FQuat::Identity, deltaTime / _interpolationRotationTime).GetNormalized();
         }
         else
         {
            _interpolationRotationOffset = FQuat::Identity;
         }

         // Test for reaching the end
         if (_interpolationLocationOffset.IsNearlyZero(1e-2f) && _interpolationRotationOffset.Equals(FQuat::Identity, 1e-5f))
         {
            _interpolationLocationOffset = FVector::ZeroVector;
            _interpolationRotationOffset = FQuat::Identity;
            _interpolationComplete = true;
         }

         // Apply result
         if (UpdatedComponent)
         {
            const FVector NewRelTranslation = UpdatedComponent->GetComponentToWorld().InverseTransformVectorNoScale(_interpolationLocationOffset) + _interpolationInitialLocationOffset;
            if (_interpolationRotation)
            {
               const FQuat NewRelRotation = _interpolationRotationOffset * _interpolationInitialRotationOffset;
               interpComponent->SetRelativeLocationAndRotation(NewRelTranslation, NewRelRotation);
            }
            else
            {
               interpComponent->SetRelativeLocation(NewRelTranslation);
            }
         }
      }
      else
      {
         ResetInterpolation();
         _interpolationComplete = true;
      }
   }

   // Might be done interpolating and want to disable tick
   if (_interpolationComplete && bAutoUpdateTickRegistration && (UpdatedComponent == nullptr))
   {
      UpdateTickRegistration();
   }
}

void UTATFlyingPatrolFollowingMovementComponent::MoveInterpolationTarget(const FVector& newLocation,
   const FRotator& newRotation)
{
   
   if (!UpdatedComponent)
   {
      return;
   }

   bool handledMovement = false;
   if (_interpolationMovement)
   {
      if (USceneComponent* interpComponent = _GetInterpolatedComponent())
      {
         // Avoid moving the child, it will interpolate later
         const FRotator interpRelativeRotation = interpComponent->GetRelativeRotation();
         FScopedPreventAttachedComponentMove scopedChildNoMove(interpComponent);

         // Update interp offset
         const FVector oldLocation = UpdatedComponent->GetComponentLocation();
         const FVector newToOldVector = (oldLocation - newLocation);
         _interpolationLocationOffset += newToOldVector;

         // Enforce distance limits
         if (newToOldVector.SizeSquared() > FMath::Square(_interpolationLocationSnapToTargetDistance))
         {
            _interpolationLocationOffset = FVector::ZeroVector;
         }
         else if (_interpolationLocationOffset.SizeSquared() > FMath::Square(_interpolationLocationMaxLagDistance))
         {
            _interpolationLocationOffset = _interpolationLocationMaxLagDistance * _interpolationLocationOffset.GetSafeNormal();
         }

         // Handle rotation
         if (_interpolationRotation)
         {
            const FQuat oldRotation = UpdatedComponent->GetComponentQuat();
            _interpolationRotationOffset = (newRotation.Quaternion().Inverse() * oldRotation) * _interpolationRotationOffset;
         }
         else
         {
            // If not interpolating rotation, we should allow the component to rotate.
            // The absolute flag will get restored by the scoped move.
            interpComponent->SetUsingAbsoluteRotation(false);
            interpComponent->SetRelativeRotation_Direct(interpRelativeRotation);
            _interpolationRotationOffset = FQuat::Identity;
         }

         // Move the root
         UpdatedComponent->SetRelativeLocationAndRotation(newLocation, newRotation);
         handledMovement = true;
         _interpolationComplete = false;
      }
      else
      {
         ResetInterpolation();
         _interpolationComplete = true;
      }
   }

   if (!handledMovement)
   {
      UpdatedComponent->SetRelativeLocationAndRotation(newLocation, newRotation);
   }
}

void UTATFlyingPatrolFollowingMovementComponent::ResetInterpolation()
{
   if (USceneComponent* interpComponent = _GetInterpolatedComponent())
   {
      interpComponent->SetRelativeLocationAndRotation(_interpolationInitialLocationOffset, _interpolationInitialRotationOffset);
   }

   _interpolationLocationOffset = FVector::ZeroVector;
   _interpolationRotationOffset = FQuat::Identity;
   _interpolationComplete = true;
}

void UTATFlyingPatrolFollowingMovementComponent::SetInterpolatedComponent(USceneComponent* component)
{
   if (component == _GetInterpolatedComponent())
   {
      return;
   }

   if (component)
   {
      ResetInterpolation();
      _interpolatedComponentPtr = component;
      _interpolationInitialLocationOffset = component->GetRelativeLocation();
      _interpolationInitialRotationOffset = component->GetRelativeRotation().Quaternion();
      _interpolationComplete = false;
   }
   else
   {
      ResetInterpolation();
      _interpolatedComponentPtr = nullptr;
      _interpolationInitialLocationOffset = FVector::ZeroVector;
      _interpolationInitialRotationOffset = FQuat::Identity;
      _interpolationComplete = true;
   }
}

USceneComponent* UTATFlyingPatrolFollowingMovementComponent::_GetInterpolatedComponent() const
{
   return _interpolatedComponentPtr.Get();
}

void UTATFlyingPatrolFollowingMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps( OutLifetimeProps );
   DOREPLIFETIME( UTATFlyingPatrolFollowingMovementComponent, _currentPauseTime );
   DOREPLIFETIME( UTATFlyingPatrolFollowingMovementComponent, _patrolIndex );
}
