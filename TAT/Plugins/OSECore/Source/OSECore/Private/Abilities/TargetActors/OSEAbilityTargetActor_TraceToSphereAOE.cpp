// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "Abilities/TargetActors/OSEAbilityTargetActor_TraceToSphereAOE.h"

//ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/TargetActors/ConeTargetHelpers.h"

//ue4
#include "GameFramework/Pawn.h"
#include "WorldCollision.h"
#include "Engine/OverlapResult.h"
#include "Abilities/GameplayAbility.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_TraceToSphereAOE)

AOSEAbilityTargetActor_TraceToSphereAOE::AOSEAbilityTargetActor_TraceToSphereAOE(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
}

void AOSEAbilityTargetActor_TraceToSphereAOE::PerformOverlap(TArray<TWeakObjectPtr<AActor>>& result, bool positionForPreview)
{
   AActor* sourceActor = SourceActor;
   FVector traceStart;
   FVector traceEnd;
   UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(sourceActor, StartLocation, MaxRange, traceStart, traceEnd);

   FCollisionQueryParams params(SCENE_QUERY_STAT(OSEAbilityTargetActor_TraceToSphereAOE), /*bTraceComplex*/ false);
   params.bReturnPhysicalMaterial = false;
   params.AddIgnoredActor(sourceActor);

   FVector sphereCenter = traceEnd;
   FHitResult traceHit;
   if (GetWorld()->LineTraceSingleByProfile(traceHit, traceStart, traceEnd, TraceProfile.Name, params))
   {
      sphereCenter = traceHit.Location;
   }

   TArray<FOverlapResult> overlaps;
   GetWorld()->OverlapMultiByObjectType(overlaps, sphereCenter, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(SphereRadius), params);

#if ENABLE_DRAW_DEBUG
   if (bDebug)
   {
      DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Blue);
      DrawDebugSphere(GetWorld(), sphereCenter, SphereRadius, 10, FColor::Emerald);
   }
#endif // ENABLE_DRAW_DEBUG

   using FActorAndDistance = TTuple<AActor*, float>;
   using FActorDistances = TArray<FActorAndDistance, TInlineAllocator<8>>;
   FActorDistances actorDistances;

   for (int32 i = 0; i < overlaps.Num(); ++i)
   {
      APawn* pawnActor = Cast<APawn>(overlaps[i].GetActor());
      if (pawnActor && Filter.FilterPassesForActor(pawnActor) && 
         !actorDistances.ContainsByPredicate([pawnActor](const FActorAndDistance& e) { return e.Key == pawnActor;}))
      {
         actorDistances.Emplace(pawnActor, FVector::Distance(pawnActor->GetActorLocation(), sphereCenter));
      }
   }

   // Keep N closest targets to center of sphere
   if (MaxTargets > 0 && actorDistances.Num() > MaxTargets)
   {
      actorDistances.Sort([](const FActorAndDistance& a, const FActorAndDistance& b) { return a.Value < b.Value; });
   }

   // hypothetical occlusion filtering that can short circuit when found enough

   const int32 foundCount = MaxTargets > 0 ? FMath::Min(actorDistances.Num(), MaxTargets) : actorDistances.Num();
   result.Reserve(foundCount);
   for (int32 i = 0; i < foundCount; ++i)
   {
      result.Add(actorDistances[i].Key);
   }

   if (positionForPreview)
   {
      SetActorLocationAndRotation(sphereCenter, sourceActor->GetActorRotation());
   }
}

