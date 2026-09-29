// (c) 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tentacle/OSETentacleState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSETentacleState)


float FOSETentacleState::UpdateLength(float target, float speed, float deltaTime)
{
   if (deltaTime > 0)
   {
      // Clamp the speed if we'd overshoot our target
      const float fullSpeed = (target - Length) / deltaTime;
      const float clampedSpeed = FMath::Min(speed, FMath::Abs(fullSpeed));
      const float actualSpeed = clampedSpeed * FMath::Sign(fullSpeed);
      Length += (actualSpeed * deltaTime);
   }

   // Returns the distance from the target
   return FMath::Abs(target - Length);
}

void FOSETentacleState::UpdateComponentSpaceLocation()
{
   _componentSpaceLocation.Reset();

   if (HitResult.IsValidBlockingHit())
   {
      if (UPrimitiveComponent* primComponent = HitResult.GetComponent())
      {
         const FTransform& componentToWorld = primComponent->GetComponentTransform();
         _componentSpaceLocation = componentToWorld.InverseTransformPosition(HitResult.Location);
      }
   }
}

FVector FOSETentacleState::GetWorldSpaceLocation() const
{
   if (_componentSpaceLocation.IsSet())
   {
      if (UPrimitiveComponent* primComponent = HitResult.GetComponent())
      {
         const FTransform& componentToWorld = primComponent->GetComponentTransform();
         return componentToWorld.TransformPosition(*_componentSpaceLocation);
      }
   }

   return HitResult.Location;
}
