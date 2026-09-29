// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Traversal/Mantle/OSELedgeQuery.h"
#include "Traversal/Mantle/OSELedgeSettings.h"
#include "Traversal/Mantle/OSELedgeState.h"

#include "WorldCollision.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSELedgeQuery)

namespace LedgeCVars
{
   static float MinWallDistance = 50.0f;
   FAutoConsoleVariableRef CVarMinWallDistance(
      TEXT("OSE.Ledge.MinWallDistance"),
      MinWallDistance,
      TEXT("Minimum distance from the initial wall impact (helps to ensure the animation lines up and helps with uneven / protruding geometry)."),
      ECVF_Default);

   static float ClearanceTraceBufferAmount = 25.0f;
   FAutoConsoleVariableRef CVarClearanceTraceBufferAmount(
      TEXT("OSE.Ledge.ClearanceTraceBufferAmount"),
      ClearanceTraceBufferAmount,
      TEXT("Reduces the height and diameter of the capsule by this amount when doing clearance checks. This reduces the likelihood of grazing hits."),
      ECVF_Default);

   static float OverheadTestHeight = 50.0f;
   FAutoConsoleVariableRef CVarOverheadTestExtent(
      TEXT("OSE.Ledge.OverheadTestHeight"),
      OverheadTestHeight,
      TEXT("The overhead sweep height. ledge detection will extend the capsule up by this ammount."),
      ECVF_Default);

   static float VerticalTraceBufferAmount = 5.0f;
   FAutoConsoleVariableRef CVarVerticalTraceBufferAmount(
      TEXT("OSE.Ledge.VerticalTraceBufferAmount"),
      VerticalTraceBufferAmount,
      TEXT("Extend the start and end vertical trace points this amount to detect hits despite precision issues (in some circumstances, precisely placed geo may not register as a blocking hit)."),
      ECVF_Default);

   static float LedgeDepth = 10.0f;
   FAutoConsoleVariableRef CVarLedgeDepth(
      TEXT("OSE.Ledge.LedgeDepth"),
      LedgeDepth,
      TEXT("How deep to test for ledges."),
      ECVF_Default);
}


// Traces for a mantle location given a character. The velocity of the character
// is used for look-ahead projection.
FOSELedgeQueryResult UOSELedgeQuery::TraceMantleCharacter(
   const FOSELedgeSettings& ledgeSettings,
   const ACharacter* Character)
{
   if (Character == nullptr)
      return FOSELedgeQueryResult();

   return TraceMantleCharacterWithVelocity(
      ledgeSettings,
      Character,
      Character->GetVelocity());
}

FOSELedgeQueryResult UOSELedgeQuery::TraceShimmyCharacterWithVelocity(
   const FOSELedgeSettings& ledgeSettings, 
   const class ACharacter* Character, 
   const FRotator& MoveRotation, 
   const FVector& Velocity)
{
   if (Character == nullptr)
      return FOSELedgeQueryResult();

   return TraceShimmyCapsuleWithVelocity(
      ledgeSettings,
      Character->GetCapsuleComponent(),
      Character->GetViewRotation(),
      MoveRotation,
      Velocity);
}

// Traces for a mantle location given a character and the specified velocity
// for look-ahead projection.
FOSELedgeQueryResult UOSELedgeQuery::TraceMantleCharacterWithVelocity(
   const FOSELedgeSettings& ledgeSettings,
   const ACharacter* Character,
   const FVector& Velocity)
{
   if (Character == nullptr)
      return FOSELedgeQueryResult();

   return TraceMantleCapsuleWithVelocity(
      ledgeSettings,
      Character->GetCapsuleComponent(),
      Character->GetViewRotation(),
      Velocity);
}

// Traces for a mantle location given a capsule component and the specified velocity
// for look-ahead projection.
FOSELedgeQueryResult UOSELedgeQuery::TraceMantleCapsuleWithVelocity(
   const FOSELedgeSettings& ledgeSettings,
   const class UCapsuleComponent* Capsule,
   const FRotator& SearchRotation,
   const FVector& Velocity)
{
   FOSELedgeQueryResult LocationData;

   if (Capsule == nullptr || Capsule->GetWorld() == nullptr)
      return LocationData;

   LocationData.CapsuleRadius = Capsule->GetScaledCapsuleRadius();
   LocationData.CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
   LocationData.QueryLocation = Capsule->GetComponentLocation();
   LocationData.QueryRotation = Capsule->GetComponentQuat();
   LocationData.QueryVelocity = Velocity;

  return TraceMantle(ledgeSettings, LocationData, Capsule, SearchRotation, Velocity);
}

FOSELedgeQueryResult UOSELedgeQuery::TraceShimmyCapsuleWithVelocity(
   const FOSELedgeSettings& ledgeSettings,
   const class UCapsuleComponent* Capsule, 
   const FRotator& SearchRotation,
   const FRotator& MoveRotation,
   const FVector& Velocity)
{
   FOSELedgeQueryResult LocationData;

   if (Capsule == nullptr || Capsule->GetWorld() == nullptr)
      return LocationData;

   LocationData.CapsuleRadius = Capsule->GetScaledCapsuleRadius();
   LocationData.CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
   LocationData.QueryLocation = Capsule->GetComponentLocation();
   LocationData.QueryRotation = Capsule->GetComponentQuat();
   LocationData.QueryVelocity = Velocity;

   // Search direction is always level
   const FVector SearchDirection = MoveRotation.Vector().GetSafeNormal2D();

   // Search distance is (v*t)+h where:
   //    v is velocity projected along search direction
   //    t is look ahead time
   //    h is capsule half height
   const float SearchDistance = ledgeSettings.ShimmyDistance.Max;

   // back off the start position so that it doesn't immediately collide with the current mantle surface
   const FVector traceStart = LocationData.QueryLocation + LocationData.QueryRotation.RotateVector(-FVector::ForwardVector * ledgeSettings.ShimmyEase);
   const FVector traceEnd = traceStart + (SearchDirection * SearchDistance);
   FVector shimmyStart = traceEnd;

   FHitResult clearanceHitResult;
   // Do the initial sweep
   if (SweepMantleSingle(
      clearanceHitResult, Capsule, traceStart, traceEnd, 
      Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius()))
   {
      if (clearanceHitResult.Distance > ledgeSettings.ShimmyDistance.Min)
      {
         shimmyStart = clearanceHitResult.Location;
      }
      else
      {
         return LocationData;
      }
   }
   LocationData.QueryLocation = shimmyStart;
   return TraceMantle(ledgeSettings, LocationData, Capsule, SearchRotation, Velocity);
}

FOSELedgeQueryResult UOSELedgeQuery::TraceFromLedgeCharacter(const FOSELedgeSettings& ledgeSettings, const class ACharacter* Character, const struct FOSELedgeState& CurrentLedge)
{
   if (Character == nullptr)
      return FOSELedgeQueryResult();

   return TraceFromLedgeCapsule(
      ledgeSettings,
      Character->GetCapsuleComponent(),
      CurrentLedge);
}


FOSELedgeQueryResult UOSELedgeQuery::TraceFromLedgeCapsule(const FOSELedgeSettings& ledgeSettings, const UCapsuleComponent* Capsule, const FOSELedgeState& CurrentLedge)
{
   FOSELedgeQueryResult LocationData;

   if (Capsule == nullptr || Capsule->GetWorld() == nullptr)
      return LocationData;

   LocationData.CapsuleRadius = Capsule->GetScaledCapsuleRadius();
   LocationData.CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
   LocationData.QueryLocation = Capsule->GetComponentLocation();
   LocationData.QueryRotation = Capsule->GetComponentQuat();
   LocationData.QueryVelocity = FVector::ZeroVector;

   //first sweep back and up -- finding a clear path up.
   FVector traceStart  = LocationData.QueryLocation;
   FVector traceEnd = traceStart - CurrentLedge.MountTarget.AnchorDirection* ledgeSettings.ClimbOffset.Max + FVector::UpVector * ledgeSettings.ClimbHeight.Max;

   //DrawDebugCapsule(Capsule->GetWorld(), traceStart, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentQuat(), FColor::Green, false, 10.0f, (uint8)'\000', 0.5f);
   //DrawDebugCapsule(Capsule->GetWorld(), traceEnd, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentQuat(), FColor::Orange, false, 10.0f, (uint8)'\000', 0.5f);


   FHitResult backwardsHit;
   if (SweepMantleSingle(
      backwardsHit, Capsule,
      traceStart, traceEnd,
      Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius()))
   {
      traceEnd = backwardsHit.Location;
     // DrawDebugCapsule(Capsule->GetWorld(), traceEnd, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentQuat(), FColor::Yellow, false, 10.0f, (uint8)'\000', 0.5f);
   }

   //sweep forwards
   traceStart = traceEnd;
   traceEnd = traceStart + CurrentLedge.MountTarget.AnchorDirection * (ledgeSettings.ClimbOffset.Max - ledgeSettings.ClimbOffset.Min);
   //DrawDebugCapsule(Capsule->GetWorld(), traceStart, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentQuat(), FColor::Magenta, false, 10.0f, (uint8)'\000', 0.5f);
   //DrawDebugCapsule(Capsule->GetWorld(), traceEnd, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentQuat(), FColor::Purple, false, 10.0f, (uint8)'\000', 0.5f);

   if (!SweepMantleSingle(
      LocationData.StartHitResult, Capsule,
      traceStart, traceEnd,
      Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius()))
   {
      // No hits, so there is nothing higher to climb on.
      // TODO: This is dumb
      FRotator searchRotation = FRotationMatrix::MakeFromX(CurrentLedge.MountTarget.AnchorDirection).Rotator();
      return TraceMantleCapsuleWithVelocity(ledgeSettings, Capsule, searchRotation, CurrentLedge.MountTarget.AnchorDirection);
   }

   // We hit an object; make sure it falls within our facing angle threshold
   LocationData.FinalHitResult = LocationData.StartHitResult;
   const FVector ContactNorm = -LocationData.StartHitResult.Normal;
   const FVector ContactPerp = FVector::CrossProduct(FVector::UpVector, ContactNorm);
   const FVector SearchDirection = CurrentLedge.MountTarget.AnchorDirection;

   // Only doing this on 2D initial search. Ensuring that we start at a minimum distance from the surface,
   // ensuring that if an impact is on the top or bottom of the capsule we back up such that the cylinder
   // sides are planar to the impact.
   {
      const float minDistFromImpact = FMath::Max(LedgeCVars::MinWallDistance, LocationData.CapsuleRadius);
      const float impactDist = FMath::Max(0.0f, (LocationData.StartHitResult.ImpactPoint - LocationData.StartHitResult.Location) | SearchDirection);
      const float impactDiff = FMath::Max(0.0f, minDistFromImpact - impactDist);
      LocationData.StartHitResult.Location -= SearchDirection * impactDiff;
   }

   const float DotNorm = SearchDirection | ContactNorm;
   const float DotPerp = SearchDirection | ContactPerp;

   const float surfaceAngle = FMath::RadiansToDegrees(FMath::Acos(DotNorm) * FMath::Sign(DotPerp));
   if (FMath::Abs(surfaceAngle) > ledgeSettings.MaxAngleFacing)
   {
      // Angle delta too high
      return LocationData;
   }

   // Min and max Z values for valid mantle range
   const float CapsuleBottom = traceStart.Z - LocationData.CapsuleHalfHeight;
   const float MantleHeightMin = CapsuleBottom + ledgeSettings.ClimbHeight.Min;
   const float MantleHeightMax = CapsuleBottom + ledgeSettings.ClimbHeight.Max;

   {
      const float kLedgeDepth = LedgeCVars::LedgeDepth;

      // Min and max search positions are offset along the search direction by the radius of the capsule
      FVector ProjectedOffset = LocationData.StartHitResult.ImpactPoint + (SearchDirection * (kLedgeDepth * 0.5));
      FVector ProjectedMin(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMin);
      FVector ProjectedMax(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMax);

      // In phase two, we find all valid mantle locations along this ray. We do multiple iterations to
      // account for colliders with holes (think windows in walls). Each subsequent iteration uses a
      // shorter ray for the cast.
      TArray< FHitResult > ValidMantleList;
      for (int32 i = 0; i < 2; ++i)
      {
         GatherAllValidMantleLocations(
            ValidMantleList, ledgeSettings, LocationData,
            Capsule, ProjectedMin, ProjectedMax,
            kLedgeDepth, kLedgeDepth);

         // Our valid hit locations are sorted by height delta. We select the first item, which represents
         // the closest valid mantle location.
         if (ValidMantleList.Num() > 0)
         {
            LocationData.LedgeHitResult = ValidMantleList[0];
            LocationData.LedgeResult = true;
            break;
         }

         // Reduce the length of the ray by half for subsequent iterations
         const float NewDelta = (ProjectedMax.Z - ProjectedMin.Z) * 0.5f;
         ProjectedMax.Z = ProjectedMin.Z + NewDelta;
      }
   }
   // Min and max search positions are offset along the search direction by the radius of the capsule
   FVector ProjectedOffset = LocationData.StartHitResult.ImpactPoint + (SearchDirection * LocationData.CapsuleRadius);
   FVector ProjectedMin(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMin);
   FVector ProjectedMax(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMax);

   // In phase three, we find all valid mantle locations along this ray. We do multiple iterations to
   // account for colliders with holes (think windows in walls). Each subsequent iteration uses a
   // shorter ray for the cast.
   TArray< FHitResult > ValidMantleList;
   for (int32 i = 0; i < 2; ++i)
   {
      GatherAllValidMantleLocations(
         ValidMantleList, ledgeSettings, LocationData,
         Capsule, ProjectedMin, ProjectedMax,
         Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius());

      // Our valid hit locations are sorted by height delta. We select the first item, which represents
      // the closest valid mantle location.
      if (ValidMantleList.Num() > 0)
      {
         LocationData.FinalHitResult = ValidMantleList[0];
         LocationData.MantleResult = true;
         break;
      }

      // Reduce the length of the ray by half for subsequent iterations
      const float NewDelta = (ProjectedMax.Z - ProjectedMin.Z) * 0.5f;
      ProjectedMax.Z = ProjectedMin.Z + NewDelta;
   }

   return LocationData;
}

FOSELedgeQueryResult UOSELedgeQuery::TraceMantle(const FOSELedgeSettings& ledgeSettings, FOSELedgeQueryResult& LocationData, const class UCapsuleComponent* Capsule, const FRotator& SearchRotation, const FVector& Velocity)
{
   // Search direction is always level
   const FVector SearchDirection = SearchRotation.Vector().GetSafeNormal2D();

   // Search distance is (v*t)+h where:
   //    v is velocity projected along search direction
   //    t is look ahead time
   //    h is capsule half height
   const float SearchDistance = ((SearchDirection | Velocity) * ledgeSettings.LookAheadTime) + LocationData.CapsuleHalfHeight;

   // Trace start and end points
   const FVector traceStart = LocationData.QueryLocation;
   const FVector traceEnd = (SearchDirection * SearchDistance) + traceStart;

   // Do the initial sweep
   if (!SweepMantleSingle(
      LocationData.StartHitResult, Capsule,
      traceStart, traceEnd,
      Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius()))
   {
      // Do an offset test over the players head, turns out people have arms that go over their head
      const FVector overheadOffset = FVector::UpVector * LedgeCVars::OverheadTestHeight;
      if (!SweepMantleSingle(
         LocationData.StartHitResult, Capsule,
         traceStart + overheadOffset, traceEnd + overheadOffset,
         Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius()))
      {
         // No hits
         return LocationData;
      }
   }

   // We hit an object; make sure it falls within our facing angle threshold
   LocationData.FinalHitResult = LocationData.StartHitResult;
   const FVector ContactNorm = -LocationData.StartHitResult.Normal.GetSafeNormal2D();
   const FVector ContactPerp = FVector::CrossProduct(FVector::UpVector, ContactNorm);
   const FVector SearchDirection2D = SearchDirection.GetSafeNormal2D();

   // Only doing this on 2D initial search. Ensuring that we start at a minimum distance from the surface,
   // ensuring that if an impact is on the top or bottom of the capsule we back up such that the cylinder
   // sides are planar to the impact.
   {
      const float minDistFromImpact = FMath::Max(LedgeCVars::MinWallDistance, LocationData.CapsuleRadius);
      const float impactDist = FMath::Max(0.0f, (LocationData.StartHitResult.ImpactPoint - LocationData.StartHitResult.Location) | SearchDirection2D);
      const float impactDiff = FMath::Max(0.0f, minDistFromImpact - impactDist);
      LocationData.StartHitResult.Location -= SearchDirection2D * impactDiff;
   }

   const float DotNorm = SearchDirection2D | ContactNorm;
   const float DotPerp = SearchDirection2D | ContactPerp;

   const float surfaceAngle = FMath::RadiansToDegrees(FMath::Acos(DotNorm) * FMath::Sign(DotPerp));
   if (FMath::Abs(surfaceAngle) > ledgeSettings.MaxAngleFacing)
   {
      // Angle delta too high
      return LocationData;
   }

   // Min and max Z values for valid mantle range
   const float CapsuleBottom = traceStart.Z - LocationData.CapsuleHalfHeight;
   const float MantleHeightMin = CapsuleBottom + ledgeSettings.EntranceHeight.Min;
   const float MantleHeightMax = CapsuleBottom + ledgeSettings.EntranceHeight.Max;

   {
      const float kLedgeDepth = LedgeCVars::LedgeDepth;

      // Min and max search positions are offset along the search direction by the radius of the capsule
      FVector ProjectedOffset = LocationData.StartHitResult.ImpactPoint + (SearchDirection * (kLedgeDepth *0.5));
      FVector ProjectedMin(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMin);
      FVector ProjectedMax(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMax);

      // In phase two, we find all valid mantle locations along this ray. We do multiple iterations to
      // account for colliders with holes (think windows in walls). Each subsequent iteration uses a
      // shorter ray for the cast.
      TArray< FHitResult > ValidMantleList;
      for (int32 i = 0; i < 2; ++i)
      {
         GatherAllValidMantleLocations(
            ValidMantleList, ledgeSettings, LocationData,
            Capsule, ProjectedMin, ProjectedMax,
            kLedgeDepth, kLedgeDepth);

         // Our valid hit locations are sorted by height delta. We select the first item, which represents
         // the closest valid mantle location.
         if (ValidMantleList.Num() > 0)
         {
            LocationData.LedgeHitResult = ValidMantleList[0];
            LocationData.LedgeResult = true;
            break;
         }

         // Reduce the length of the ray by half for subsequent iterations
         const float NewDelta = (ProjectedMax.Z - ProjectedMin.Z) * 0.5f;
         ProjectedMax.Z = ProjectedMin.Z + NewDelta;
      }
   }
   // Min and max search positions are offset along the search direction by the radius of the capsule
   FVector ProjectedOffset = LocationData.StartHitResult.ImpactPoint + (SearchDirection * LocationData.CapsuleRadius);
   FVector ProjectedMin(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMin);
   FVector ProjectedMax(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMax);

   // In phase three, we find all valid mantle locations along this ray. We do multiple iterations to
   // account for colliders with holes (think windows in walls). Each subsequent iteration uses a
   // shorter ray for the cast.
   TArray< FHitResult > ValidMantleList;
   for (int32 i = 0; i < 2; ++i)
   {
      GatherAllValidMantleLocations(
         ValidMantleList, ledgeSettings, LocationData,
         Capsule, ProjectedMin, ProjectedMax, 
         Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius());

      // Our valid hit locations are sorted by height delta. We select the first item, which represents
      // the closest valid mantle location.
      if (ValidMantleList.Num() > 0)
      {
         LocationData.FinalHitResult = ValidMantleList[0];
         LocationData.MantleResult = true;
         break;
      }

      // Reduce the length of the ray by half for subsequent iterations
      const float NewDelta = (ProjectedMax.Z - ProjectedMin.Z) * 0.5f;
      ProjectedMax.Z = ProjectedMin.Z + NewDelta;
   }

   // Let's do a final inward and downward trace in case we were jumping/climbing up an angled wall
   if (LocationData.LedgeResult)
   {
      //#define DEBUG_LEDGE_TRACES

      FCollisionShape traceShape;
      FCollisionQueryParams queryParams;
      FCollisionResponseParams responseParams;
      ECollisionChannel collisionChannel;
      const UWorld* world = GetMantleSweepParams(traceShape, queryParams, responseParams, collisionChannel, Capsule, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius());
      if (!IsValid(world))
      {
         return LocationData;
      }

#ifdef DEBUG_LEDGE_TRACES
      DrawDebugSphere(world, LocationData.LedgeHitResult.ImpactPoint, 8, 16, FColor::Blue);
#endif //DEBUG_LEDGE_TRACES

      FVector fromPointToPlayer = (LocationData.QueryLocation - LocationData.LedgeHitResult.ImpactPoint);
      FVector fromPointToPlayerNormal2D = fromPointToPlayer.GetSafeNormal2D();
      FVector fromPlayerToPointNormal2D = -fromPointToPlayerNormal2D;
      FVector lastInwardTraceStart = LocationData.LedgeHitResult.ImpactPoint + (fromPointToPlayerNormal2D * SearchDistance);
      FVector lastInwardTraceEnd = lastInwardTraceStart + (fromPlayerToPointNormal2D * SearchDistance);

      FCollisionShape sphereShape = FCollisionShape::MakeSphere(8);
      world->SweepSingleByChannel(LocationData.LedgeHitResult, lastInwardTraceStart, lastInwardTraceEnd, FQuat::Identity, collisionChannel, sphereShape, queryParams, responseParams);

#ifdef DEBUG_LEDGE_TRACES
      DrawDebugSphere(world, LocationData.LedgeHitResult.ImpactPoint, 8, 16, FColor::Green);
#endif //DEBUG_LEDGE_TRACES

      // One last downward trace to make sure our hit location isn't too close to the edge
      FVector lastDownwardTraceStart = LocationData.LedgeHitResult.ImpactPoint + (fromPlayerToPointNormal2D * (Capsule->GetScaledCapsuleRadius() * 0.5f));
      lastDownwardTraceStart.Z += Capsule->GetScaledCapsuleHalfHeight();
      FVector lastDownwardTraceEnd = lastDownwardTraceStart + (-FVector::UpVector * SearchDistance);

      world->SweepSingleByChannel(LocationData.LedgeHitResult, lastDownwardTraceStart, lastDownwardTraceEnd, FQuat::Identity, collisionChannel, sphereShape, queryParams, responseParams);

#ifdef DEBUG_LEDGE_TRACES
      DrawDebugSphere(world, LocationData.LedgeHitResult.ImpactPoint, 8, 16, FColor::Magenta);
#endif //DEBUG_LEDGE_TRACES
   }

   if (LocationData.MantleResult && !LocationData.LedgeResult)
   {
      LocationData.LedgeHitResult = LocationData.FinalHitResult;
   }

   return LocationData;
}

void UOSELedgeQuery::GatherAllValidMantleLocations(
   TArray< FHitResult >& ValidMantleList,
   const FOSELedgeSettings& ledgeSettings,
   const FOSELedgeQueryResult& LocationData,
   const class UCapsuleComponent* Capsule,
   const FVector& ProjectedMin,
   const FVector& ProjectedMax,
   const float capsuleHalfHeight,
   const float capsuleRadius)
 {
   ValidMantleList.Empty(1);

   
   FVector OriginalTraceStart = ProjectedMax + (FVector::UpVector * capsuleHalfHeight);
   FVector OriginalTraceEnd = ProjectedMin + (FVector::UpVector * capsuleHalfHeight);
   const FVector traceDir = (OriginalTraceEnd - OriginalTraceStart).GetSafeNormal();

   FVector adjustedTraceEnd = OriginalTraceEnd;
   FVector TraceEnd = OriginalTraceEnd;
   bool bUpdated = true;
   const float kEndProjBuffer = FMath::Max(0.0f, LedgeCVars::VerticalTraceBufferAmount);

   // There could be multiple gaps between the query start and a valid ledge/mantle point.
   // Each iteration we sweep up to find a gap ceiling and cast down from there. Subsequent 
   // iterations will sweep up from that point (plus a small ammount). If we run out of search 
   // space, find a valid mantle, or just don't find a new gap, we're done.
   while(bUpdated && ValidMantleList.IsEmpty() && TraceEnd.Z < (OriginalTraceStart.Z - capsuleHalfHeight*2))
   {
      FVector TraceStart = OriginalTraceStart;
      //Advance our end position up
      TraceEnd = adjustedTraceEnd;

      // Extend the end trace point so that we can detect hits despite precision issues (in some
      // circumstances, precisely placed geo may not register as a blocking hit)
      TraceStart -= traceDir * kEndProjBuffer;
      TraceEnd += traceDir * kEndProjBuffer;
   
      // Perform a sweep from bottom to the top first. If we have a blocking hit, use the location
      // as our new top starting point. This helps to identify valid mantle locations that might
      // otherwise be missed, such as window ledges and other odd geometry.
      {
         FHitResult upwardsHit;
         if (SweepMantleMulti(upwardsHit, Capsule, TraceEnd, TraceStart, capsuleHalfHeight, capsuleRadius))
         {
            adjustedTraceEnd = TraceStart = upwardsHit.Location;
            adjustedTraceEnd -= traceDir * kEndProjBuffer;
            
      
         }
         else
         {
            bUpdated = false;
         }
      }

      //Back off the trace start. The sweep can lack precision. The sweep up can find nothing, but the sweep down with the same start/end reversed can be penetrated
      TraceStart += traceDir * kEndProjBuffer;

      FHitResult CandidateHit;
      if (SweepMantleSingle(CandidateHit, Capsule, TraceStart, TraceEnd, capsuleHalfHeight, capsuleRadius))
      {
         // Verifies that this is a valid _final_ result for mantling by 
         if (!IsValidMantleResult(CandidateHit, Cast<APawn>(Capsule->GetOwner())) )
         {
            return;
         }

         // Make sure the player can stand without falling off
         ACharacter* charOwner = Cast<ACharacter>(Capsule->GetOwner());
         UCharacterMovementComponent* moveComp = IsValid(charOwner) ? Cast<UCharacterMovementComponent>(charOwner->GetMovementComponent()) : NULL;
         if (IsValid(moveComp) && moveComp->IsWalkable(CandidateHit))
         {
            // Now we need to verify that we can reach the desired location. We perform a clearance test
            // of sweeps expected to fail and not find any blocking hits. The first sweep is upwards from
            // the player to a mid point. The midpoint is the start location XY and the Z of the final
            // location. Then, from this mid location to the final location. This ensures there are no
            // other obstructions blocking this candidate location.
            FVector clearanceLocStart = LocationData.StartHitResult.Location;
            FVector clearanceLocMid = LocationData.StartHitResult.Location;
            FVector clearanceLocEnd = CandidateHit.Location;
            clearanceLocMid.Z = clearanceLocEnd.Z;

            // Reduced scale for height and radius when doing clearance checks. Total buffer distance in cm
            // is applied as a scale to the half height and radius, so it is divided by half to ensure the
            // total buffer distance is accounted for.
            const float kClearanceBufferAmount = LedgeCVars::ClearanceTraceBufferAmount;
            const float kClearanceHeightScale  = (capsuleHalfHeight - (kClearanceBufferAmount * 0.5f)) / capsuleHalfHeight;
            const float kClearanceRadiusScale  = (capsuleRadius - (kClearanceBufferAmount * 0.5f)) / capsuleRadius;

            // Upwards sweep from start to mid point. We use a shorter, narrower capsule to avoid grazing hits.
            FHitResult clearanceHitStartToMid;
            if (!SweepMantleSingle(clearanceHitStartToMid, Capsule, clearanceLocStart, clearanceLocMid, capsuleHalfHeight * kClearanceHeightScale, capsuleRadius * kClearanceRadiusScale))
            {
               // Horizontal sweep from mid to final point. We use a shorter, narrower capsule to avoid grazing hits.
               FHitResult clearanceHitMidToEnd;
               if (!SweepMantleSingle(clearanceHitMidToEnd, Capsule, clearanceLocMid, clearanceLocEnd, capsuleHalfHeight * kClearanceHeightScale, capsuleRadius * kClearanceRadiusScale))
               {
                  ValidMantleList.Add(CandidateHit);
               }
            }
         }
      }
   }
}

// Utility method to get sweep params
const UWorld* UOSELedgeQuery::GetMantleSweepParams(
   FCollisionShape& TraceShape,
   FCollisionQueryParams& QueryParams,
   FCollisionResponseParams& ResponseParams,
   ECollisionChannel& CollisionChannel,
   const UCapsuleComponent* Capsule,
   const float Height,
   const float Radius)
{
   const UWorld* World = (Capsule != nullptr) ? Capsule->GetWorld() : nullptr;
   if (World != nullptr)
   {
      // Trace shape
     const FVector CapsuleExtents(Radius, Radius, Height);
      TraceShape = FCollisionShape::MakeCapsule(CapsuleExtents);

      // Query and response params
      QueryParams = FCollisionQueryParams(SCENE_QUERY_STAT(UOSELedgeQuery), false, Capsule->GetOwner());
      Capsule->InitSweepCollisionParams(QueryParams, ResponseParams);

      // Additional parameters
      QueryParams.bFindInitialOverlaps = false;
      QueryParams.bIgnoreTouches = true;
      QueryParams.bReturnFaceIndex = false;
      QueryParams.bReturnPhysicalMaterial = true;

      // Collision channel
      CollisionChannel = Capsule->GetCollisionObjectType();
   }

   return World;
}

// Utility method for single capsule sweep
bool UOSELedgeQuery::SweepMantleSingle(
   FHitResult& HitResult,
   const UCapsuleComponent* Capsule,
   const FVector& TraceStart,
   const FVector& TraceEnd,
   const float Height,
   const float Radius)
{
   FCollisionShape TraceShape;
   FCollisionQueryParams QueryParams;
   FCollisionResponseParams ResponseParams;
   ECollisionChannel CollisionChannel;
   const UWorld* World = GetMantleSweepParams(TraceShape, QueryParams, ResponseParams, CollisionChannel,
      Capsule, Height, Radius);

   if (World == nullptr)
   {
      return false;
   }

   if (!World->SweepSingleByChannel(
      HitResult, TraceStart, TraceEnd, Capsule->GetComponentQuat(),
      CollisionChannel, TraceShape, QueryParams, ResponseParams))
   {
      return false;
   }

   // Success
   return HitResult.IsValidBlockingHit();
}

// Utility method for single capsule sweep
bool UOSELedgeQuery::SweepMantleMulti(
   FHitResult& HitResult,
   const UCapsuleComponent* Capsule,
   const FVector& TraceStart,
   const FVector& TraceEnd,
   const float Height,
   const float Radius)
{
   FCollisionShape TraceShape;
   FCollisionQueryParams QueryParams;
   FCollisionResponseParams ResponseParams;
   ECollisionChannel CollisionChannel;
   const UWorld* World = GetMantleSweepParams(TraceShape, QueryParams, ResponseParams, CollisionChannel,
      Capsule, Height, Radius);

   if (World == nullptr)
   {
      return false;
   }

   TArray<struct FHitResult> hits;
   if (!World->SweepMultiByChannel(
      hits, TraceStart, TraceEnd, Capsule->GetComponentQuat(),
      CollisionChannel, TraceShape, QueryParams, ResponseParams))
   {
      return false;
   }

   for (FHitResult& hit : hits)
   {
      if(hit.IsValidBlockingHit())
      {
         //success
         HitResult = hit;
         return true;
      }
   }
   return false;
}

// Utility method to validate final hit results. Uses similar logic to UCharacterMovementComponent::CanStepUp
// without the movement mode check, since we want to support mantle while falling.
bool UOSELedgeQuery::IsValidMantleResult(
   const FHitResult& hitResult,
   APawn* pawnOwner)
{
   if (!hitResult.IsValidBlockingHit())
   {
      return false;
   }

   if (pawnOwner)
   {
      // No component for "fake" hits when we are on a known good base
      if (const UPrimitiveComponent* hitComponent = hitResult.Component.Get())
      {
         if (!hitComponent->CanCharacterStepUp(pawnOwner))
         {
            return false;
         }
      }

      // No actor for "fake" hits when we are on a known good base
      if (const AActor* hitActor = hitResult.GetActor())
      {
         if (!hitActor->CanBeBaseForCharacter(pawnOwner))
         {
            return false;
         }
      }
   }

   return true;
}

// Filters hit results using to ensure only valid final hit results are considered
bool UOSELedgeQuery::FilterValidMantleResults(
   TArray<FHitResult>& outHits,
   APawn* pawnOwner)
{
   outHits.RemoveAll([pawnOwner](const FHitResult& hitResult) { return !IsValidMantleResult(hitResult, pawnOwner); });
   return (outHits.Num() > 0);
}

