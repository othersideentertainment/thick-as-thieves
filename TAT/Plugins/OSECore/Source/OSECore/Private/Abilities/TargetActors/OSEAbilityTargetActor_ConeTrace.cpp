// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TargetActors/OSEAbilityTargetActor_ConeTrace.h"

// ose
#include "Abilities/TargetActors/ConeTargetHelpers.h"
#include "OSECommon.h"

//ue4
#include "Abilities/GameplayAbility.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_ConeTrace)


AOSEAbilityTargetActor_ConeTrace::AOSEAbilityTargetActor_ConeTrace()
   : Super()
{
}

FHitResult AOSEAbilityTargetActor_ConeTrace::PerformTrace(AActor* inSourceActor)
{
   ConeTargetHelpers::FConeTraceParams traceParams;
   traceParams.SourceActor = inSourceActor;
   traceParams.ObjectQueryParams = MakeObjectQueryParams();
   traceParams.LineOfSightProfile = GetTraceProfile();
   traceParams.MaxRange = MaxRange;
   traceParams.HalfAngle = HalfAngle;
   traceParams.StartLocation = StartLocation;
   traceParams.DrawDebug = bDebug;

   ConeTargetHelpers::FConeTraceResult result = ConeTargetHelpers::DoConeTrace(traceParams, Filter);
   APawn* foundActor = result.FoundActor;

   FHitResult returnHitResult;
   returnHitResult.Init(result.TraceStart, result.TraceEnd);
   returnHitResult.HitObjectHandle = foundActor;
   returnHitResult.bBlockingHit = foundActor != nullptr;

   if (AGameplayAbilityWorldReticle* localReticleActor = ReticleActor.Get())
   {
      const bool hitActor = foundActor != nullptr;
      const FVector reticleLocation = (hitActor && localReticleActor->bSnapToTargetedActor) ? foundActor->GetActorLocation() : returnHitResult.Location;

      localReticleActor->SetActorLocation(reticleLocation);
      localReticleActor->SetIsTargetAnActor(hitActor);
   }

   return returnHitResult;
}

FCollisionObjectQueryParams AOSEAbilityTargetActor_ConeTrace::MakeObjectQueryParams() const
{
   return FCollisionObjectQueryParams(ECC_Pawn);
}

FCollisionProfileName AOSEAbilityTargetActor_ConeTrace::GetTraceProfile() const
{
   return TraceProfile;
}

