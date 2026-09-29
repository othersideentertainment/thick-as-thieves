// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TargetActors/ConeTargetHelpers.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue4
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "Math/Vector.h"
#include "DrawDebugHelpers.h"

class APawn;

namespace ConeTargetHelpers
{
   FVector FindClosestPointOnPawn(APawn* pawnActor, const FVector& traceStart, const FVector& traceEnd, FVector* closestPointOnTraceSegment /*= nullptr*/)
   {
      const FVector pawnLocation = pawnActor->GetActorLocation();
      const FVector closestPointOnSegment = FMath::ClosestPointOnSegment(pawnLocation, traceStart, traceEnd);

      if (closestPointOnTraceSegment)
      {
         *closestPointOnTraceSegment = closestPointOnSegment;
      }

      // treat capsule like cylinder for laziness
      float radius, height;
      pawnActor->GetSimpleCollisionCylinder(radius, height);
      const float halfHeight = height * 0.5f;
      
      FVector delta2d = closestPointOnSegment - pawnLocation;
      delta2d.Z = 0;

      FVector result = pawnLocation;
      result.Z = FMath::Clamp(closestPointOnSegment.Z, result.Z - halfHeight, result.Z + halfHeight);
      result += delta2d.GetClampedToMaxSize2D(radius);
      return result;
   }

   bool FLineOfSightContext::DoLineOfSightTrace(const FVector& traceStart, const FVector& traceEnd) const
   {
      const bool result = !World->LineTraceTestByProfile(traceStart, traceEnd, TraceProfile.Name, Params);
#if ENABLE_DRAW_DEBUG
      if (DrawDebug)
      {
         DrawDebugLine(World, traceStart, traceEnd, result ? FColor::Cyan : FColor::Red, false, 0.5f);
      }
#endif // ENABLE_DRAW_DEBUG
      return result;
   }

   bool FLineOfSightContext::HasLineOfSight(const FVector& traceStart, APawn* pawnActor) const
   {
      // try center of mass
      if (DoLineOfSightTrace(traceStart, pawnActor->GetActorLocation())) return true;

      // also try top of head
      float radius, height;
      pawnActor->GetSimpleCollisionCylinder(radius, height);
      return DoLineOfSightTrace(traceStart, pawnActor->GetActorLocation() + FVector(0, 0, height * 0.5f));
   }

   FConeTraceResult DoConeTrace(const FConeTraceParams& params, TFunctionRef<bool(const AActor* actor)> filter)
   {
      const AActor* sourceActor = params.SourceActor;
      FVector traceStart;
      FVector traceEnd;
      UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(sourceActor, params.StartLocation, params.MaxRange, traceStart, traceEnd);

      const FVector heading = (traceEnd - traceStart).GetSafeNormal();
      const float fullAngle = params.HalfAngle * 2.f;
      const float angleAsRad = FMath::DegreesToRadians(fullAngle);
      const float cosTargetAngle = FMath::Cos(angleAsRad);
      const float sinTargetAngle = FMath::Sin(angleAsRad);

      FCollisionQueryParams queryParams(SCENE_QUERY_STAT(OSEAbilityTargetActor_ConeTrace), /*bTraceComplex*/ false);
      queryParams.bReturnPhysicalMaterial = false;
      queryParams.AddIgnoredActor(params.SourceActor);

      const FSphere boundingSphereForCone = FMath::ComputeBoundingSphereForCone(
         traceStart,
         heading,
         static_cast<FSphere::FReal>(params.MaxRange),
         static_cast<FSphere::FReal>(cosTargetAngle),
         static_cast<FSphere::FReal>(sinTargetAngle));

#if ENABLE_DRAW_DEBUG
      if (params.DrawDebug)
      {
         DrawDebugCone(sourceActor->GetWorld(), 
            traceStart, 
            heading, 
            params.MaxRange,
            angleAsRad,
            angleAsRad,
            10,
            FColor::Emerald);
         DrawDebugSphere(sourceActor->GetWorld(), traceStart, params.MaxRange, 32, FColor::Red, false, 0.f);
         DrawDebugSphere(sourceActor->GetWorld(), boundingSphereForCone.Center, boundingSphereForCone.W, 32, FColor::Yellow, false, 0.f);
      }
#endif
      
      TArray<FOverlapResult> overlaps;
      sourceActor->GetWorld()->OverlapMultiByObjectType(overlaps, boundingSphereForCone.Center, FQuat::Identity, params.ObjectQueryParams, FCollisionShape::MakeSphere(boundingSphereForCone.W), queryParams);

#if ENABLE_DRAW_DEBUG
      if (params.DrawDebug)
      {
         DrawDebugLine(sourceActor->GetWorld(), traceStart, traceEnd, FColor::Blue, false);
         DrawDebugCone(sourceActor->GetWorld(),
            traceStart, 
            heading, 
            params.MaxRange,
            angleAsRad,
            angleAsRad, 
            10,
            FColor::Emerald);
      }
#endif // ENABLE_DRAW_DEBUG

      float bestRankScore = MAX_flt;
      APawn* foundActor = nullptr;
      ConeTargetHelpers::FLineOfSightContext losContext(sourceActor->GetWorld(), queryParams, params.LineOfSightProfile, params.DrawDebug);

      const float maxDistanceSquared = FMath::Square(params.MaxRange);
      for (int32 i = 0; i < overlaps.Num(); ++i)
      {
         APawn* pawnActor = Cast<APawn>(overlaps[i].GetActor());
         if (!pawnActor || !filter(pawnActor))
         {
            continue;
         }

         FVector closestPointOnTraceToPawn = FVector::ZeroVector;
         const FVector pawnLocation = ConeTargetHelpers::FindClosestPointOnPawn(pawnActor, traceStart, traceEnd, &closestPointOnTraceToPawn);
         const FVector startToPawn = pawnLocation - traceStart;
         
         const float distSquared = startToPawn.SizeSquared();
         if (distSquared > maxDistanceSquared)
         {
#if ENABLE_DRAW_DEBUG
            if (params.DrawDebug)
            {
               DrawDebugLine(sourceActor->GetWorld(), traceStart, pawnLocation, FColor::Red);
            }
#endif
            continue;
         }
#if ENABLE_DRAW_DEBUG
         if (params.DrawDebug)
         {
            DrawDebugLine(sourceActor->GetWorld(), traceStart, pawnLocation, FColor::Green);
         }
#endif
         const float pawnDirectionHeadingOverlap = startToPawn.GetSafeNormal() | heading;

         float pawnRankScore = 0;
         switch (params.RankCriteria)
         {
            case EConeTraceRankCriteria::ShortestDistanceToStart:
               pawnRankScore = startToPawn.SizeSquared();
            break;

            case EConeTraceRankCriteria::ShortestDistanceToTraceLine:
               pawnRankScore = (pawnLocation - closestPointOnTraceToPawn).SizeSquared();
            break;

            case EConeTraceRankCriteria::SmallestAngle:
               // Here we negate the dot product: smaller score means a better score, and dot products are the opposite
               pawnRankScore = pawnDirectionHeadingOverlap * -1.0f;
            break;

            default:
               checkNoEntry();
               break;
         }
         if (pawnRankScore > bestRankScore) continue;

         // Is this still needed? It _should_ be within the bounding sphere of the cone, 
         // I think might be needed, if its close by but above / below the source location. So within the sphere but out
         // of the cone.
         if (cosTargetAngle > pawnDirectionHeadingOverlap)
         {
            continue;
         }

         queryParams.ClearIgnoredSourceObjects();
         queryParams.AddIgnoredActor(sourceActor);
         queryParams.AddIgnoredActor(pawnActor);

         if (!losContext.HasLineOfSight(traceStart, pawnActor))
         {
            continue;
         }

         foundActor = pawnActor;
         bestRankScore = pawnRankScore;
      }

      FConeTraceResult result;
      result.FoundActor = foundActor;
      result.TraceStart = traceStart;
      result.TraceEnd = traceEnd;
      return result;
   }
}
