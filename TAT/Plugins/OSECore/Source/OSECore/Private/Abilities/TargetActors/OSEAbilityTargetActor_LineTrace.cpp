// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TargetActors/OSEAbilityTargetActor_LineTrace.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "OSECommon.h"

//ue4
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_LineTrace)

AOSEAbilityTargetActor_LineTrace::AOSEAbilityTargetActor_LineTrace()
   : Super()
{
}

FHitResult AOSEAbilityTargetActor_LineTrace::PerformTrace(AActor* inSourceActor)
{
   // Call the OSE-specific targeting function, bTraceAffectsAimPitch is ignored as it doesn't make sense for the targeting model

   FHitResult returnHitResult;
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

void AOSEAbilityTargetActor_LineTrace::ConfirmTargetingAndContinue()
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

