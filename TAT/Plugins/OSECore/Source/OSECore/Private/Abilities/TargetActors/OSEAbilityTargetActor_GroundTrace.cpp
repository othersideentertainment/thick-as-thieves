// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TargetActors/OSEAbilityTargetActor_GroundTrace.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "OSECommon.h"

//ue4
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_GroundTrace)


AOSEAbilityTargetActor_GroundTrace::AOSEAbilityTargetActor_GroundTrace()
   : Super()
{
}

bool AOSEAbilityTargetActor_GroundTrace::ShouldProduceTargetData() const
{
   // Workaround for ShouldProduceTargetData being false on the server for AI, because it is erroneously only checking
   // MasterPC, which is explicitly a PlayerController. This is preferable to setting ShouldProduceTargetDataOnServer,
   // as that will have other, potentially undesirable side-effects on the server.
   // TODO: If this get fiddly, consider engine mod?
   // NOTE: If you change this, also update the other ShouldProduceTargetData methods with the same fix.
   return Super::ShouldProduceTargetData() || (OwningAbility && OwningAbility->GetActorInfo().IsLocallyControlled());
}

void AOSEAbilityTargetActor_GroundTrace::ConfirmTargeting()
{
   const bool willProduceConfirmation = IsConfirmTargetingAllowed();

   Super::ConfirmTargeting();

   // it looks like the base class assumes that a confirm press that fails should just kill targeting, but in our case
   // we want it to continue when there's a failure.  so we'll re-bind to the events that are unbound in the superclass.
   if (!willProduceConfirmation)
   {
      const FGameplayAbilityActorInfo* const actorInfo = OwningAbility->GetCurrentActorInfo();
      UAbilitySystemComponent* const asc = actorInfo->AbilitySystemComponent.Get();
      if (asc)
      {
         if (actorInfo->IsLocallyControlled())
         {
            asc->GenericLocalConfirmCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor::ConfirmTargeting);
         }
         else
         {
            FGameplayAbilitySpecHandle handle = OwningAbility->GetCurrentAbilitySpecHandle();
            FPredictionKey predKey = OwningAbility->GetCurrentActivationInfo().GetActivationPredictionKey();

            GenericConfirmHandle = asc->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GenericConfirm, handle, predKey).AddUObject(this, &AGameplayAbilityTargetActor::ConfirmTargeting);

            if (asc->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::GenericConfirm, handle, predKey))
            {
               return;
            }
         }
      }
   }
}

void AOSEAbilityTargetActor_GroundTrace::CancelTargeting()
{
   Super::CancelTargeting();
}

void AOSEAbilityTargetActor_GroundTrace::StartTargeting(UGameplayAbility* inAbility)
{
   Super::StartTargeting(inAbility);
}

// Largely the same as the engine version, but:
// - Added additional debug drawing
// - Added support for casting down from the player's view instead
// - Optionally ignore overlaps, generally speaking ground targeting things only wants to deal in blocks
FHitResult AOSEAbilityTargetActor_GroundTrace::PerformTrace(AActor* inSourceActor)
{
   static const bool kTraceComplex = false;

   FCollisionQueryParams params(SCENE_QUERY_STAT(AGameplayAbilityTargetActor_GroundTrace), kTraceComplex);
   params.bReturnPhysicalMaterial = true;
   params.AddIgnoredActor(inSourceActor);
   params.bIgnoreTouches = IgnoreOverlaps;

   FVector traceStart = StartLocation.GetTargetingTransform().GetLocation();// InSourceActor->GetActorLocation();
   FVector traceEnd;
   AimWithPlayerController(inSourceActor, params, traceStart, traceEnd);      //Effective on server and launching client only

   // ------------------------------------------------------

   //Use a line trace initially to see where the player is actually pointing
   FHitResult returnHitResult;
   LineTraceWithFilter(returnHitResult, inSourceActor->GetWorld(), Filter, traceStart, traceEnd, TraceProfile.Name, params);

#if ENABLE_DRAW_DEBUG
   if (bDebug)
   {
      DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Magenta, false);
   }
#endif // ENABLE_DRAW_DEBUG

   // cache off our aim trace
   FVector aimTraceStart = traceStart;
   FVector aimTraceEnd = traceEnd;

   //Default to end of trace line if we don't hit anything.
   if (!returnHitResult.bBlockingHit)
   {
      returnHitResult.Location = traceEnd;
   }

   //Second trace, straight down. Consider using InSourceActor->GetWorld()->NavigationSystem->ProjectPointToNavigation() instead of just going straight down in the case of movement abilities (flag/bool).
   traceStart = returnHitResult.Location - (traceEnd - traceStart).GetSafeNormal();      //Pull back very slightly to avoid scraping down walls
   traceEnd = traceStart;
   traceStart.Z += CollisionHeightOffset;
   traceEnd.Z -= 99999.0f;
   LineTraceWithFilter(returnHitResult, inSourceActor->GetWorld(), Filter, traceStart, traceEnd, TraceProfile.Name, params);
   //if (!ReturnHitResult.bBlockingHit) then our endpoint may be off the map. Hopefully this is only possible in debug maps.

#if ENABLE_DRAW_DEBUG
   if (bDebug)
   {
      DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Silver, false);
   }
#endif // ENABLE_DRAW_DEBUG

   bLastTraceWasGood = true;      //So far, we're good. If we need a ground spot and can't find one, we'll come back.

   //Use collision shape to find a valid ground spot, if appropriate
   if (CollisionShape.ShapeType != ECollisionShape::Line)
   {
      returnHitResult.Location.Z += CollisionHeightOffset;      //Rise up out of the ground

      if (AllowTargetingOnLedges)
      {
         // when we allow targeting on ledges we do our traces downward from the player's eye
         traceStart = aimTraceStart;
         traceEnd = aimTraceEnd;
         traceStart.Z += CollisionHeightOffset;
      }
      else
      {
         // otherwise we do our traces downward from the ground where the ability target ends, or collided
         traceStart = inSourceActor->GetActorLocation();
         traceEnd = returnHitResult.Location;
         traceStart.Z += CollisionHeightOffset;
      }

#if ENABLE_DRAW_DEBUG
      if (bDebug)
      {
         DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Yellow, false);
      }
#endif // ENABLE_DRAW_DEBUG

      bLastTraceWasGood = AdjustCollisionResultForShapeWithVisibility(traceStart, traceEnd, params, returnHitResult);
      if (bLastTraceWasGood)
      {
         returnHitResult.Location.Z -= CollisionHeightOffset;   //Undo the artificial height adjustment
      }
   }

   if (AGameplayAbilityWorldReticle* localReticleActor = ReticleActor.Get())
   {
      localReticleActor->SetIsTargetValid(bLastTraceWasGood);
      localReticleActor->SetActorLocation(returnHitResult.Location);
   }

   // Reset the trace start so the target data uses the correct origin
   returnHitResult.TraceStart = StartLocation.GetTargetingTransform().GetLocation();

   return returnHitResult;
}

// Largely a copy of the engine version, but:
// - Added additional debug drawing
// - We optionally ensure that our character can see our target before considering it valid (which is considerably more expensive)
bool AOSEAbilityTargetActor_GroundTrace::AdjustCollisionResultForShapeWithVisibility(const FVector originalStartPoint, const FVector originalEndPoint, const FCollisionQueryParams params, FHitResult& outHitResult) const
{
   UWorld* world = GetWorld();
   
   //Pull back toward player to find a better spot, accounting for the width of our object
   FVector movement = (originalEndPoint - originalStartPoint);
   FVector movementDirection = movement.GetSafeNormal();
   float movementMagnitude2D = movement.Size2D();

#if ENABLE_DRAW_DEBUG
   if (bDebug)
   {
      if (CollisionShape.ShapeType == ECollisionShape::Capsule)
      {
         DrawDebugCapsule(world, originalEndPoint, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Black);
      }
      else
      {
         DrawDebugSphere(world, originalEndPoint, CollisionRadius, 8, FColor::Black);
      }
   }
#endif // ENABLE_DRAW_DEBUG

   if (movementMagnitude2D <= (CollisionRadius * 2.0f))
   {
      return false;      //Bad case!
   }

   //TODO This increment value needs to ramp up - the first few increments should be small, then we should start moving in larger steps. A few ideas for this:
   //1. Use a curve! Even one defined by a hardcoded formula would be fine, this isn't something that should require user tuning, or that the user should really know/care about.
   //2. Use larger increments as the object is further from the player/camera, since the user can't really perceive precision at long range.
   float incrementSize = FMath::Clamp<float>(CollisionRadius * 0.5f, 20.0f, 50.0f);
   float lerpIncrement = incrementSize / movementMagnitude2D;
   FHitResult localResult;
   FVector traceStart;
   FVector traceEnd;
   for (float lerpValue = CollisionRadius / movementMagnitude2D; lerpValue < 1.0f; lerpValue += lerpIncrement)
   {
      traceEnd = traceStart = originalEndPoint - (lerpValue * movement);
      traceEnd.Z -= 99999.0f;
      SweepWithFilter(localResult, world, Filter, traceStart, traceEnd, FQuat::Identity, CollisionShape, TraceProfile.Name, params);
      
#if ENABLE_DRAW_DEBUG
      if (bDebug)
      {
         DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Blue, false); // OSE Added
      }
#endif // ENABLE_DRAW_DEBUG
      
      if (!localResult.bStartPenetrating)
      {
         if (!localResult.bBlockingHit || (localResult.HitObjectHandle.IsValid() && Cast<APawn>(localResult.GetActor())))
         {
            //Off the map, or hit an actor
#if ENABLE_DRAW_DEBUG
            if (bDebug)
            {
               if (CollisionShape.ShapeType == ECollisionShape::Capsule)
               {
                  DrawDebugCapsule(world, localResult.Location, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Yellow);
               }
               else
               {
                  DrawDebugSphere(world, localResult.Location, CollisionRadius, 8, FColor::Yellow);
               }
            }
#endif // ENABLE_DRAW_DEBUG
            continue;
         }
#if ENABLE_DRAW_DEBUG
         if (bDebug)
         {
            if (CollisionShape.ShapeType == ECollisionShape::Capsule)
            {
               DrawDebugCapsule(world, localResult.Location, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Green);
            }
            else
            {
               DrawDebugSphere(world, localResult.Location, CollisionRadius, 8, FColor::Green);
            }
         }
#endif // ENABLE_DRAW_DEBUG

         // Ensure that our player can see the object
         if (EnsureTargetVisibility)
         {
            FVector startVisTraceLoc = originalStartPoint;
            FVector endVisTraceLoc = localResult.ImpactPoint;
            FHitResult visibilityHitResult;
            LineTraceWithFilter(visibilityHitResult, world, Filter, startVisTraceLoc, endVisTraceLoc, TraceProfile.Name, params);
            FVector impactPoint = visibilityHitResult.ImpactPoint;

#if ENABLE_DRAW_DEBUG
            if (bDebug)
            {
               DrawDebugLine(GetWorld(), startVisTraceLoc, endVisTraceLoc, FColor::Cyan, false);
            }
#endif // ENABLE_DRAW_DEBUG

            static const float kTolerance = 0.1f;
            if (visibilityHitResult.bBlockingHit && !(FMath::IsNearlyEqual(impactPoint.X, endVisTraceLoc.X, kTolerance) &&
                FMath::IsNearlyEqual(impactPoint.Y, endVisTraceLoc.Y, kTolerance) &&
                FMath::IsNearlyEqual(impactPoint.Z, endVisTraceLoc.Z, kTolerance)))
            {
#if ENABLE_DRAW_DEBUG
               if (bDebug)
               {
                  DrawDebugSphere(world, visibilityHitResult.Location, 20.0f, 8, FColor::Cyan);
               }
#endif // ENABLE_DRAW_DEBUG

               // keep searching for something we can see
               continue;
            }
         }         

         //TODO: Test for flat ground. Concept: Test four corners and the center, make triangles out of the center and adjacent corner points. Check normal.Z of triangles against a minimum Z value.

         outHitResult = localResult;
         return true;
      }
#if ENABLE_DRAW_DEBUG
      if (bDebug)
      {
         if (CollisionShape.ShapeType == ECollisionShape::Capsule)
         {
            DrawDebugCapsule(world, traceStart, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Red);
         }
         else
         {
            DrawDebugSphere(world, traceStart, CollisionRadius, 8, FColor::Red);
         }
      }
#endif // ENABLE_DRAW_DEBUG
   }
   return false;
}

