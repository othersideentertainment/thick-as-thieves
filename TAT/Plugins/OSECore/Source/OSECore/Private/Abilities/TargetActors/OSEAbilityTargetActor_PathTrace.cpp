// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TargetActors/OSEAbilityTargetActor_PathTrace.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "OSECommon.h"

// ue
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_PathTrace)

AOSEAbilityTargetActor_PathTrace::AOSEAbilityTargetActor_PathTrace()
   : Super()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
}

FHitResult AOSEAbilityTargetActor_PathTrace::PerformTrace(AActor* inSourceActor)
{
   FHitResult returnHitResult;

   UWorld* world = GetWorld();
   check(world != nullptr);

   // Call the OSE-specific targeting function, bTraceAffectsAimPitch is ignored as it doesn't make sense for the targeting model

   UOSEAbilityFunctionLibrary::PerformTargetTrace(returnHitResult, inSourceActor, StartLocation, MaxRange, TraceProfile, Filter, bDebug);

   if (AGameplayAbilityWorldReticle* localReticleActor = ReticleActor.Get())
   {
      const bool hitActor = (returnHitResult.bBlockingHit && (returnHitResult.GetActor() != NULL));
      const FVector reticleLocation = (hitActor && localReticleActor->bSnapToTargetedActor) ? returnHitResult.GetActor()->GetActorLocation() : returnHitResult.Location;

      localReticleActor->SetActorLocation(reticleLocation);
      localReticleActor->SetIsTargetAnActor(hitActor);
   }

   if (bPreQuantizeHitResult)
   {
      UOSECommon::PreQuantizeHitResult(returnHitResult);
   }

   return returnHitResult;
}

void AOSEAbilityTargetActor_PathTrace::ConfirmTargetingAndContinue()
{
   // NOTE: Duplicate of base class code, but without turning off bDebug before making our trace so we can actually see it -- why do they do that?!
   
   check(ShouldProduceTargetData());
   if (SourceActor)
   {
      //bDebug = false;
      FHitResult hitResult = PerformTrace(SourceActor);
      bDebug = false;
      FGameplayAbilityTargetDataHandle Handle = MakeTargetData(hitResult);
      if (_AllowHitAsTarget(hitResult))
      {
         TargetDataReadyDelegate.Broadcast(Handle);
      }
      else
      {
         CanceledDelegate.Broadcast(Handle);
      }
   }
}

bool AOSEAbilityTargetActor_PathTrace::_PerformPathTrace(FHitResult& outHitResult, const FVector& targetLocation) const
{
   UWorld* world = GetWorld();
   check(world != nullptr);

   FCollisionQueryParams queryParams{};
   FVector pathTraceStart = PathStartWorldLocation;
   FVector pathTraceEnd = targetLocation;
   bool addPathTraceOffset = false;
   if (FVector::DistSquared(pathTraceStart, pathTraceEnd) >= FMath::Square(4.0f))
   {
      static constexpr float offset = 0.25f;
      const FVector pathTraceDir = (pathTraceEnd - pathTraceStart).GetSafeNormal();
      pathTraceStart += pathTraceDir * offset;
      pathTraceEnd -= pathTraceDir * offset;
      addPathTraceOffset = true;
   }

   LineTraceWithFilter(outHitResult, world, Filter, pathTraceStart, pathTraceEnd, TraceProfile.Name, queryParams);

   if (bDebug)
   {
      static constexpr bool persistentLines = false;
      static constexpr float drawDuration = 5.0f;

      if (addPathTraceOffset)
      {
         DrawDebugLine(world, PathStartWorldLocation, pathTraceStart, FColor(120, 120, 120), persistentLines, drawDuration);
         DrawDebugLine(world, pathTraceEnd, targetLocation, FColor(120, 120, 120), persistentLines, drawDuration);
      }

      if (outHitResult.bBlockingHit)
      {
         DrawDebugLine(world, pathTraceStart, outHitResult.Location, FColor::Green, persistentLines, drawDuration);
         DrawDebugPoint(world, outHitResult.Location, 10.0f, FColor::Red, persistentLines, drawDuration);
         DrawDebugLine(world, outHitResult.Location, pathTraceEnd, FColor::Yellow, persistentLines, drawDuration);
      }
      else
      {
         DrawDebugLine(world, pathTraceStart, pathTraceEnd, FColor::Green, persistentLines, drawDuration);
      }
   }

   return outHitResult.bBlockingHit;
}

bool AOSEAbilityTargetActor_PathTrace::_AllowHitAsTarget(const FHitResult& hit) const
{
   return true;
}

void AOSEAbilityTargetActor_PathTrace::Tick(float deltaSeconds)
{
   // N.B. Deliberately not calling Super::Tick, since we are overriding the behavior in AGameplayAbilityTargetActor_Trace::Tick
   // to get access to the hit result. Still calling AActor::Tick to ensure we get BP tick events, or run any latent actions tied to us
   AActor::Tick(deltaSeconds);

   if (SourceActor && SourceActor->GetLocalRole() != ROLE_SimulatedProxy)
   {
      FHitResult targetHitResult = PerformTrace(SourceActor);
      const bool isValidTarget = _AllowHitAsTarget(targetHitResult);
      FHitResult pathTraceHitResult{};
      const bool isValidPath = !_PerformPathTrace(pathTraceHitResult, targetHitResult.Location);
      UpdateTargetingVisuals(targetHitResult, isValidTarget, pathTraceHitResult, isValidPath);
   }
}
