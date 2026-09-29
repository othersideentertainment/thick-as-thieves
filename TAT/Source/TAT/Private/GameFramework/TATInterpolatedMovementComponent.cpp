// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/TATInterpolatedMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInterpolatedMovementComponent)

UTATInterpolatedMovementComponent::UTATInterpolatedMovementComponent()
{
   _interpolationComplete = true;

   _interpMovement = false;
   _interpRotation = false;
   _interpolationComplete = true;
   _interpLocationTime = 0.100f;
   _interpRotationTime = 0.050f;
   _interpLocationMaxLagDistance = 300.0f;
   _interpLocationSnapToTargetDistance = 500.0f;
}

void UTATInterpolatedMovementComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (_interpMovement && !_interpolationComplete)
   {
      QUICK_SCOPE_CYCLE_COUNTER(STAT_TATInterpolatedMovementComponent_TickInterpolation);
      _TickInterpolation(deltaTime);
   }
}

void UTATInterpolatedMovementComponent::SetInterpolatedComponent(USceneComponent* component)
{
   if (component == GetInterpolatedComponent())
   {
      return;
   }

   if (component)
   {
      ResetInterpolation();
      _interpolatedComponentPtr = component;
      _interpInitialLocationOffset = component->GetRelativeLocation();
      _interpInitialRotationOffset = component->GetRelativeRotation().Quaternion();
      _interpolationComplete = false;
   }
   else
   {
      ResetInterpolation();
      _interpolatedComponentPtr = nullptr;
      _interpInitialLocationOffset = FVector::ZeroVector;
      _interpInitialRotationOffset = FQuat::Identity;
      _interpolationComplete = true;
   }
}

USceneComponent* UTATInterpolatedMovementComponent::GetInterpolatedComponent() const
{
   return _interpolatedComponentPtr.Get();
}

void UTATInterpolatedMovementComponent::MoveInterpolationTarget(const FVector& newLocation, const FRotator& newRotation)
{
   if (!UpdatedComponent)
   {
      return;
   }

   bool handledMovement = false;
   if (_interpMovement)
   {
      if (USceneComponent* interpComponent = GetInterpolatedComponent())
      {
         // Avoid moving the child, it will interpolate later
         const FRotator interpRelativeRotation = interpComponent->GetRelativeRotation();
         FScopedPreventAttachedComponentMove scopedChildNoMove(interpComponent);

         // Update interp offset
         const FVector oldLocation = UpdatedComponent->GetComponentLocation();
         const FVector newToOldVector = (oldLocation - newLocation);
         _interpLocationOffset += newToOldVector;

         // Enforce distance limits
         if (newToOldVector.SizeSquared() > FMath::Square(_interpLocationSnapToTargetDistance))
         {
            _interpLocationOffset = FVector::ZeroVector;
         }
         else if (_interpLocationOffset.SizeSquared() > FMath::Square(_interpLocationMaxLagDistance))
         {
            _interpLocationOffset = _interpLocationMaxLagDistance * _interpLocationOffset.GetSafeNormal();
         }

         // Handle rotation
         if (_interpRotation)
         {
            const FQuat oldRotation = UpdatedComponent->GetComponentQuat();
            _interpRotationOffset = (newRotation.Quaternion().Inverse() * oldRotation) * _interpRotationOffset;
         }
         else
         {
            // If not interpolating rotation, we should allow the component to rotate.
            // The absolute flag will get restored by the scoped move.
            interpComponent->SetUsingAbsoluteRotation(false);
            interpComponent->SetRelativeRotation_Direct(interpRelativeRotation);
            _interpRotationOffset = FQuat::Identity;
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

void UTATInterpolatedMovementComponent::ResetInterpolation()
{
   if (USceneComponent* interpComponent = GetInterpolatedComponent())
   {
      interpComponent->SetRelativeLocationAndRotation(_interpInitialLocationOffset, _interpInitialRotationOffset);
   }

   _interpLocationOffset = FVector::ZeroVector;
   _interpRotationOffset = FQuat::Identity;
   _interpolationComplete = true;
}

void UTATInterpolatedMovementComponent::_TickInterpolation(float deltaTime)
{
   if (!_interpolationComplete)
   {
      if (USceneComponent* interpComponent = GetInterpolatedComponent())
      {
         // Smooth location. Interp faster when stopping.
         const float actualInterpLocationTime = Velocity.IsZero() ? 0.5f * _interpLocationTime : _interpLocationTime;
         if (deltaTime < actualInterpLocationTime)
         {
            // Slowly decay translation offset (lagged exponential smoothing)
            _interpLocationOffset = (_interpLocationOffset * (1.f - deltaTime / actualInterpLocationTime));
         }
         else
         {
            _interpLocationOffset = FVector::ZeroVector;
         }

         // Smooth rotation
         if (deltaTime < _interpRotationTime && _interpRotation)
         {
            // Slowly decay rotation offset
            _interpRotationOffset = FQuat::FastLerp(_interpRotationOffset, FQuat::Identity, deltaTime / _interpRotationTime).GetNormalized();
         }
         else
         {
            _interpRotationOffset = FQuat::Identity;
         }

         // Test for reaching the end
         if (_interpLocationOffset.IsNearlyZero(1e-2f) && _interpRotationOffset.Equals(FQuat::Identity, 1e-5f))
         {
            _interpLocationOffset = FVector::ZeroVector;
            _interpRotationOffset = FQuat::Identity;
            _interpolationComplete = true;
         }

         // Apply result
         if (UpdatedComponent)
         {
            const FVector NewRelTranslation = UpdatedComponent->GetComponentToWorld().InverseTransformVectorNoScale(_interpLocationOffset) + _interpInitialLocationOffset;
            if (_interpRotation)
            {
               const FQuat NewRelRotation = _interpRotationOffset * _interpInitialRotationOffset;
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

