// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "Abilities/TargetActors/OSEAbilityTargetActor_ConeAOE.h"

//ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/TargetActors/ConeTargetHelpers.h"

//ue4
#include "GameFramework/Pawn.h"
#include "WorldCollision.h"
#include "Engine/OverlapResult.h"
#include "Abilities/GameplayAbility.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_ConeAOE)

AOSEAbilityTargetActor_ConeAOE::AOSEAbilityTargetActor_ConeAOE(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
   bCheckLineOfSight = true;
}

// TODO: Code dupe with ConeTargetHelpers::DoConeTrace() ?
void AOSEAbilityTargetActor_ConeAOE::PerformOverlap(TArray<TWeakObjectPtr<AActor>>& result, bool positionForPreview)
{
   AActor* sourceActor = SourceActor;
   FVector traceStart;
   FVector traceEnd;
   UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(sourceActor, StartLocation, MaxRange, traceStart, traceEnd);
   const FVector heading = (traceEnd - traceStart).GetSafeNormal();
   const float cosTargetAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngle));

   FCollisionQueryParams params(SCENE_QUERY_STAT(OSEAbilityTargetActor_ConeAOE), /*bTraceComplex*/ false);
   params.bReturnPhysicalMaterial = false;
   params.AddIgnoredActor(sourceActor);

   // Get pawn overlaps with sphere, then check angles (could make a tighter sphere, but this works)
   // TODO: try FMath::ComputeBoundingSphereForCone
   TArray<FOverlapResult> overlaps;
   GetWorld()->OverlapMultiByObjectType(overlaps, traceStart, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(MaxRange), params);

#if ENABLE_DRAW_DEBUG
   if (bDebug)
   {
      DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Blue, false);
      DrawDebugCone(GetWorld(), traceStart, heading, MaxRange, FMath::DegreesToRadians(HalfAngle * 2), FMath::DegreesToRadians(HalfAngle * 2), 10, FColor::Emerald);
   }
#endif // ENABLE_DRAW_DEBUG

   for (int32 i = 0; i < overlaps.Num(); ++i)
   {
      APawn* pawnActor = Cast<APawn>(overlaps[i].GetActor());
      if (!pawnActor || result.Contains(pawnActor) || !Filter.FilterPassesForActor(pawnActor))
      {
         continue;
      }

      FVector pawnLocation = ConeTargetHelpers::FindClosestPointOnPawn(pawnActor, traceStart, traceEnd);
      FVector startToPawn = pawnLocation - traceStart;

      if (cosTargetAngle > (startToPawn.GetSafeNormal() | heading))
      {
         continue;
      }

      result.Add(pawnActor);
      params.AddIgnoredActor(pawnActor);
   }

   if (bCheckLineOfSight)
   {
      ConeTargetHelpers::FLineOfSightContext losContext(GetWorld(), params, LineOfSightTraceProfile, bDebug);
      result.RemoveAll([&losContext, &traceStart](TWeakObjectPtr<AActor>& actorPtr) {
         APawn* pawn = CastChecked<APawn>(actorPtr.Get());
         return !losContext.HasLineOfSight(traceStart, pawn);
      });
   }

   // If we want to cap the number of pawns found for design or performance reasons, we can do it here

   if (positionForPreview)
   {
      SetActorLocationAndRotation(traceStart, FRotationMatrix::MakeFromX(traceEnd - traceStart).Rotator());
   }
}

