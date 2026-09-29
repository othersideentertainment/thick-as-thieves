// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/OSEFloatingPawnMovement.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEFloatingPawnMovement)


void UOSEFloatingPawnMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
   if (ShouldSkipUpdate(DeltaTime))
   {
      return;
   }

   Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

   if (!PawnOwner || !UpdatedComponent)
   {
      return;
   }

   const AController* Controller = PawnOwner->GetController();
   //if (Controller && Controller->IsLocalController())
   {
      // apply input for local players but also for AI that's not following a navigation path at the moment
      //if (Controller->IsLocalPlayerController() == true || Controller->IsFollowingAPath() == false || bUseAccelerationForPaths)
      {
         ApplyControlInputToVelocity(DeltaTime);
      }
      // if it's not player controller, but we do have a controller, then it's AI
      // (that's not following a path) and we need to limit the speed
      if (IsExceedingMaxSpeed(MaxSpeed) == true)
      {
         Velocity = Velocity.GetUnsafeNormal() * MaxSpeed;
      }

      LimitWorldBounds();
      bPositionCorrected = false;

      // Move actor
      FVector Delta = Velocity * DeltaTime;

      if (!Delta.IsNearlyZero(1e-6f))
      {
         const FVector OldLocation = UpdatedComponent->GetComponentLocation();
         const FQuat Rotation = UpdatedComponent->GetComponentQuat();

         FHitResult Hit(1.f);
         SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);

         if (Hit.IsValidBlockingHit())
         {
            HandleImpact(Hit, DeltaTime, Delta);
            // Try to slide the remaining distance along the surface.
            SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
         }

         // Update velocity
         // We don't want position changes to vastly reverse our direction (which can happen due to penetration fixups etc)
         if (!bPositionCorrected)
         {
            const FVector NewLocation = UpdatedComponent->GetComponentLocation();
            Velocity = ((NewLocation - OldLocation) / DeltaTime);
         }
      }

      // Finalize
      UpdateComponentVelocity();
   }
};
