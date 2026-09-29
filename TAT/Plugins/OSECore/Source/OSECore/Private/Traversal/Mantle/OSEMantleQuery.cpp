// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Traversal/Mantle/OSEMantleQuery.h"
#include "WorldCollision.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMantleQuery)


#if ENABLE_VISUAL_LOG
#define TRY_VIS_LOG_CAPSULE(Capsule, LogChannel, Location, Height, Radius, Color) if (MantleCVars::ShouldUseVisualLogger){UE_VLOG_CAPSULE(Capsule->GetOwner(), LogChannel, Log, Location, Height, Radius, FQuat::Identity, Color, TEXT_EMPTY);}
#define TRY_VIS_LOG_CAPSULE_WIRE(Capsule, LogChannel, Location, Height, Radius, Color) if (MantleCVars::ShouldUseVisualLogger){UE_VLOG_WIRECAPSULE(Capsule->GetOwner(), LogChannel, Log, Location, Height, Radius, FQuat::Identity, Color, TEXT_EMPTY);}
#else
#define TRY_VIS_LOG_CAPSULE(Capsule, LogChannel, Location, Height, Radius, Color)
#define TRY_VIS_LOG_CAPSULE_WIRE(Capsule, LogChannel, Location, Height, Radius, Color)
#endif


namespace MantleCVars
{
   static float MinWallDistance = 50.0f;
   FAutoConsoleVariableRef CVarMinWallDistance(
      TEXT("OSE.Mantle.MinWallDistance"),
      MinWallDistance,
      TEXT("Minimum distance from the initial wall impact (helps to ensure the animation lines up and helps with uneven / protruding geometry)."),
      ECVF_Default);

   static float ClearanceTraceBufferAmount = 10.0f;
   FAutoConsoleVariableRef CVarClearanceTraceBufferAmount(
      TEXT("OSE.Mantle.ClearanceTraceBufferAmount"),
      ClearanceTraceBufferAmount,
      TEXT("Reduces the height and diameter of the capsule by this amount when doing clearance checks. This reduces the likelihood of grazing hits."),
      ECVF_Default);

   static float VerticalTraceBufferAmount = 5.0f;
   FAutoConsoleVariableRef CVarVerticalTraceBufferAmount(
      TEXT("OSE.Mantle.VerticalTraceBufferAmount"),
      VerticalTraceBufferAmount,
      TEXT("Extend the start and end vertical trace points this amount to detect hits despite precision issues (in some circumstances, precisely placed geo may not register as a blocking hit)."),
      ECVF_Default);

   static bool GapSearchEnabled = true;
   FAutoConsoleVariableRef CVarGapSearchEnabled(
      TEXT("OSE.Mantle.GapSearch.Enabled"),
      VerticalTraceBufferAmount,
      TEXT("Whether to look for gap to mantle into using a (experimental-ish) sweep heuristic"),
      ECVF_Default);

   static bool GapSearchRequireDownHit = true;
   FAutoConsoleVariableRef CVarGapSearchRequireDownHit(
      TEXT("OSE.Mantle.GapSearch.RequireDownHit"),
      GapSearchRequireDownHit,
      TEXT("Whether a gap needs a downwards hit to be a candidate"),
      ECVF_Default);

   static bool GapSearchDrawDebug = false;
   FAutoConsoleVariableRef CVarGapSearchDrawDebug(
      TEXT("OSE.Mantle.GapSearch.DrawDebug"),
      GapSearchDrawDebug,
      TEXT("Whether to do debug drawing for the gap search"),
      ECVF_Default);

   static float GapSearchMinFloorClearance = 5;
   FAutoConsoleVariableRef CVarGapSearchMinFloorClearance(
      TEXT("OSE.Mantle.GapSearch.MinFloorClearance"),
      GapSearchMinFloorClearance,
      TEXT("Minimum distance between upward and downward hits to be a gap candidate"),
      ECVF_Default);

   static float GapSearchCeilingAdjust = 2;
   FAutoConsoleVariableRef CVarGapSearchCeilingAdjust(
      TEXT("OSE.Mantle.GapSearch.CeilingAdjust"),
      GapSearchCeilingAdjust,
      TEXT("Distance in cm to adjust down from a ceiling hit to search downwards from"),
      ECVF_Default);
   
   static bool ShouldUseVisualLogger = false;
   FAutoConsoleVariableRef CVarVisualLogger(
      TEXT("OSE.Mantle.VisualLogger"),
      ShouldUseVisualLogger,
      TEXT("Should we log to the visual logger?"),
      ECVF_Default);
}


DEFINE_LOG_CATEGORY_STATIC(LogOSEMantle, Log, All);

// Traces for a mantle location given a character. The velocity of the character
// is used for look-ahead projection.
FOSEMantleQueryResult UOSEMantleQuery::TraceMantleCharacter(
   const FOSEMantleSettings& MantleSettings,
   const ACharacter* Character)
{
   if (Character == nullptr)
      return FOSEMantleQueryResult();

   return TraceMantleCharacterWithVelocity(
      MantleSettings,
      Character,
      Character->GetVelocity());
}

// Traces for a mantle location given a character and the specified velocity
// for look-ahead projection.
FOSEMantleQueryResult UOSEMantleQuery::TraceMantleCharacterWithVelocity(
   const FOSEMantleSettings& MantleSettings,
   const ACharacter* Character,
   const FVector& Velocity)
{
   if (Character == nullptr)
      return FOSEMantleQueryResult();

   return TraceMantleCapsuleWithVelocity(
      MantleSettings,
      Character->GetCapsuleComponent(),
      Character->GetViewRotation(),
      Velocity);
}

// Traces for a mantle location given a capsule component and the specified velocity
// for look-ahead projection.
FOSEMantleQueryResult UOSEMantleQuery::TraceMantleCapsuleWithVelocity(
   const FOSEMantleSettings& MantleSettings,
   const class UCapsuleComponent* Capsule,
   const FRotator& SearchRotation,
   const FVector& Velocity)
{
   return UOSEMantleQuery::TraceMantleCapsuleWithVelocityAndPosition(MantleSettings, Capsule, Capsule->GetComponentLocation(), SearchRotation, Velocity);
}

FOSEMantleQueryResult UOSEMantleQuery::TraceMantleCapsuleWithVelocityAndPosition(
   const FOSEMantleSettings & MantleSettings,
   const class UCapsuleComponent* Capsule,
   const FVector & StartingPosition,
   const FRotator & SearchRotation,
   const FVector & Velocity)
{
   FOSEMantleQueryResult LocationData {};

   if (Capsule == nullptr || Capsule->GetWorld() == nullptr)
      return LocationData;

   LocationData.CapsuleRadius = Capsule->GetScaledCapsuleRadius();
   LocationData.CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
   LocationData.QueryLocation = StartingPosition;
   LocationData.QueryRotation = Capsule->GetComponentQuat();
   LocationData.QueryVelocity = Velocity;

   // Search direction is always level
   const FVector SearchDirection = SearchRotation.Vector().GetSafeNormal2D();

   // Search distance is (v*t)+h where:
   //    v is velocity projected along search direction
   //    t is look ahead time
   //    h is capsule half height
   const float SearchDistance = ((SearchDirection | Velocity) * MantleSettings.LookAheadTime) + LocationData.CapsuleHalfHeight;

   // Trace start and end points
   const FVector traceStart = LocationData.QueryLocation;
   const FVector traceEnd = (SearchDirection * SearchDistance) + traceStart;

   // Do the initial sweep
   if (!SweepMantleSingle(
      LocationData.StartHitResult,
      Capsule,
      traceStart,
      traceEnd))
   {
      // No hits
      return LocationData;
   }

   // We hit an object; make sure it falls within our facing angle threshold
   LocationData.FinalHitResult = LocationData.StartHitResult;
   const FVector ContactNorm = -LocationData.StartHitResult.Normal;
   const FVector ContactPerp = FVector::CrossProduct(FVector::UpVector, ContactNorm);

   // Only doing this on 2D initial search. Ensuring that we start at a minimum distance from the surface,
   // ensuring that if an impact is on the top or bottom of the capsule we back up such that the cylinder
   // sides are planar to the impact.
   {
      const float minDistFromImpact = FMath::Max(MantleCVars::MinWallDistance, LocationData.CapsuleRadius);
      const float impactDist = FMath::Max(0.0f, (LocationData.StartHitResult.ImpactPoint - LocationData.StartHitResult.Location) | SearchDirection);
      const float impactDiff = FMath::Max(0.0f, minDistFromImpact - impactDist);
      LocationData.StartHitResult.Location -= SearchDirection * impactDiff;
   }

   const float DotNorm = SearchDirection | ContactNorm;
   const float DotPerp = SearchDirection | ContactPerp;

   const float surfaceAngle = FMath::RadiansToDegrees(FMath::Acos(DotNorm) * FMath::Sign(DotPerp));
   if (FMath::Abs(surfaceAngle) > MantleSettings.MaxAngleFacing)
   {
      // Angle delta too high
      return LocationData;
   }

   // Min and max Z values for valid mantle range
   const float CapsuleBottom = traceStart.Z - LocationData.CapsuleHalfHeight;
   const float MantleHeightMin = CapsuleBottom + MantleSettings.MinHeight;
   const float MantleHeightMax = CapsuleBottom + MantleSettings.MaxHeight;

   // Min and max search positions are offset along the search direction by the radius of the capsule
   FVector ProjectedOffset = LocationData.StartHitResult.ImpactPoint + (SearchDirection * LocationData.CapsuleRadius);
   FVector ProjectedMin(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMin);
   FVector ProjectedMax(ProjectedOffset.X, ProjectedOffset.Y, MantleHeightMax);

   // In phase two, we find all valid mantle locations along this ray. We do multiple iterations to
   // account for colliders with holes (think windows in walls). Each subsequent iteration uses a
   // shorter ray for the cast.
   TArray< FOSEMantleQueryResult > ValidMantleList;
   for (int32 i = 0; i < 2; ++i)
   {
      GatherAllValidMantleLocations(
         ValidMantleList, MantleSettings, LocationData,
         Capsule, ProjectedMin, ProjectedMax);

      // Our valid hit locations are sorted by height delta. We select the first item, which represents
      // the closest valid mantle location.
      if (ValidMantleList.Num() > 0)
      {
         return ValidMantleList[0];
      }

      // Reduce the length of the ray by half for subsequent iterations
      const float NewDelta = (ProjectedMax.Z - ProjectedMin.Z) * 0.5f;
      ProjectedMax.Z = ProjectedMin.Z + NewDelta;
   }

   return LocationData;
}

void UOSEMantleQuery::GatherAllValidMantleLocations(
   TArray< FOSEMantleQueryResult >& ValidMantleList,
   const FOSEMantleSettings& MantleSettings,
   const FOSEMantleQueryResult& LocationData,
   const class UCapsuleComponent* Capsule,
   const FVector& ProjectedMin,
   const FVector& ProjectedMax)
{
   ValidMantleList.Empty(1);

   const float capsuleHalfHeight = LocationData.CapsuleHalfHeight;
   const float capsuleRadius = LocationData.CapsuleRadius;

   FVector TraceStart = ProjectedMax + (FVector::UpVector * capsuleHalfHeight);
   FVector TraceEnd = ProjectedMin + (FVector::UpVector * capsuleHalfHeight);
   const FVector traceDir = (TraceEnd - TraceStart).GetSafeNormal();

   if(MantleCVars::GapSearchEnabled)
   {
      // Do a pair of overlap sweeps upwards and downwards, with blocking hits treated as overlaps
      // It then uses a very basic heuristics to find likely gaps.
      //
      // NOTE:
      // 1. This is not the most accurate thing, or even the most accurate use of the data. But
      //    since it tends towards false positives [with one major exception], and we check those, the risk of regression is fairly
      //    low.
      // 2. It turns out that meshes with a single mesh collision proxy around the hole do not trigger the overlap in the sweep multiple times
      //    after exiting and re-entering. Whether we want to alter our meshes to play nice with this trick is TBD, however it is also true that
      //    a complex mesh collision proxy is also worse than primitives for other reasons.
      // 3. Definitely room for improvement/re-work
      FCollisionShape traceShape = MakeCapsuleTraceShape(Capsule);
      FCollisionQueryParams queryParams;
      FCollisionResponseParams responseParams;
      ECollisionChannel collisionChannel;
      const UWorld* world = GetMantleSweepParams(queryParams, responseParams, collisionChannel,
         Capsule);
      responseParams.CollisionResponse.ReplaceChannels(ECR_Overlap, ECR_Ignore);
      responseParams.CollisionResponse.ReplaceChannels(ECR_Block, ECR_Overlap);
      queryParams.bIgnoreTouches = false;

      auto doOverlapTrace = [&](const FVector& start, const FVector& end, TArray<FHitResult>& hits, const FColor& color) {
            
         world->SweepMultiByChannel(
            hits, start, end, Capsule->GetComponentQuat(),
            collisionChannel, traceShape, queryParams, responseParams);
#if ENABLE_DRAW_DEBUG
         if (MantleCVars::GapSearchDrawDebug)
         {
            for (const FHitResult& hit : hits)
            {
               DrawDebugSphere(world, hit.Location, 25, 16, color, false, 1);
            }
         }
#endif
#if ENABLE_VISUAL_LOG
         if (MantleCVars::ShouldUseVisualLogger)
         {
            const AActor* owner = Capsule->GetOwner();
            for (const FHitResult& hit : hits)
            {
               UE_VLOG_WIRESPHERE(owner, LogOSEMantle, Log, hit.Location, 25, color, TEXT_EMPTY);
            }
         }
#endif
      };

      TArray<FHitResult> downHits;
      TArray<FHitResult> upHits;

      doOverlapTrace(TraceStart, TraceEnd, downHits, FColor::Magenta);
      doOverlapTrace(TraceEnd, TraceStart, upHits, FColor::Green);

      auto findDownHitBelow = [&](double z) -> const FHitResult* {
         for (const FHitResult& downHit : downHits)
         {
            if (downHit.Location.Z < z)
            {
               return &downHit;
            }
         }
         return nullptr;
      };

      auto hasBlockingOverlap = [Capsule](const FVector& location) {
         FCollisionShape traceShape = MakeCapsuleTraceShape(Capsule);
         FCollisionQueryParams queryParams;
         FCollisionResponseParams responseParams;
         ECollisionChannel collisionChannel;
         const UWorld* world = GetMantleSweepParams(queryParams, responseParams, collisionChannel, Capsule);
         return world->OverlapBlockingTestByChannel(location, Capsule->GetComponentQuat(), collisionChannel, traceShape, queryParams, responseParams);
      };

      for (const FHitResult& upHit : upHits)
      {
         // If there isn't a downward hit that would obviously overlap the where there is an upwards hit,
         // then there _may_ be a gap. Not the most foolproof, even with this data, but tends to fit, as
         // long as a mesh with a hole doesn't using a single mesh collision proxy. Whether that is fine
         // long-term is TBD.
         const double z = upHit.Location.Z;
         const FHitResult* downHitBelow = findDownHitBelow(z + capsuleHalfHeight);
         if ((downHitBelow == nullptr && !MantleCVars::GapSearchRequireDownHit) || 
            (downHitBelow && (z - downHitBelow->Location.Z) > MantleCVars::GapSearchMinFloorClearance))
         {
            FVector candidateLocation = upHit.Location;
            candidateLocation.Z -= 2;
#if ENABLE_DRAW_DEBUG
            if (MantleCVars::GapSearchDrawDebug)
            {
               DrawDebugCapsule(world, candidateLocation, capsuleHalfHeight, capsuleRadius, FQuat::Identity, FColor::Orange, false, 1);
            }
#endif
            TRY_VIS_LOG_CAPSULE_WIRE(Capsule->GetOwner(), LogOSEMantle, candidateLocation, capsuleHalfHeight, capsuleRadius, FColor::Orange)
            if (!hasBlockingOverlap(candidateLocation))
            {
               TraceStart = candidateLocation;
               break;
            }
         }
      }
   }

   // Extend the end trace point so that we can detect hits despite precision issues (in some
   // circumstances, precisely placed geo may not register as a blocking hit)
   {
      const float kEndProjBuffer = FMath::Max(0.0f, MantleCVars::VerticalTraceBufferAmount);
      TraceEnd += traceDir * kEndProjBuffer;
   }

   FHitResult CandidateHit;
   if (SweepMantleSingle(CandidateHit, Capsule, TraceStart, TraceEnd))
   {
      // Verifies that this is a valid _final_ result for mantling by 
      if (!IsValidMantleResult(CandidateHit, Capsule))
      {
         TRY_VIS_LOG_CAPSULE_WIRE(Capsule->GetOwner(), LogOSEMantle, CandidateHit.Location, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FColor::Red)
         return;
      }
      
      TRY_VIS_LOG_CAPSULE_WIRE(Capsule->GetOwner(), LogOSEMantle, CandidateHit.Location, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FColor::Green)

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
         // 
         // EDIT - Drew Graham (2/17/23): to allow the forward clearance sweep to adjust for vertically-lipped ledges,
         // the forward clearance sweep has been updated to perform a downward-sphere sweep from the max mantle height
         // (ProjectionMax.Z) onto the blocking geometry. The forward clearance sweep is then re-performed from there,
         // adjusted to take into account the height of the lip per the downward-sphere sweep.
         FVector clearanceLocStart = LocationData.StartHitResult.Location;
         FVector clearanceLocMid = LocationData.StartHitResult.Location;
         FVector clearanceLocEnd = CandidateHit.Location;
         clearanceLocMid.Z = clearanceLocEnd.Z;

         // Reduced scale for height and radius when doing clearance checks. Total buffer distance in cm
         // is applied as a scale to the half height and radius, so it is divided by half to ensure the
         // total buffer distance is accounted for.
         const float kClearanceBufferAmount = MantleCVars::ClearanceTraceBufferAmount;
         const float kClearanceHeightScale = (capsuleHalfHeight - (kClearanceBufferAmount * 0.5f)) / capsuleHalfHeight;
         const float kClearanceRadiusScale = (capsuleRadius - (kClearanceBufferAmount * 0.5f)) / capsuleRadius;

         // Horizontal sweep from mid to final point. We use a shorter, narrower capsule to avoid grazing hits.
         FHitResult clearanceHitMidToEnd;

         bool clearedMidToEndSweep = !SweepMantleSingle(clearanceHitMidToEnd, Capsule, clearanceLocMid, clearanceLocEnd, kClearanceHeightScale, kClearanceRadiusScale);
         if (!clearedMidToEndSweep)
         {
            // Some surfaces may have a upward-jutting lip that catches the forward capsule sweep.
            // To address this, we sweep downward upon that lip to get it's height, and continue
            // the capsule sweep forward from there
            const float lipSweepSphereRadius = capsuleRadius * kClearanceRadiusScale;
            FVector lipDownwardSweepStart = clearanceHitMidToEnd.ImpactPoint + (FVector::UpVector * capsuleHalfHeight);
            FVector lipDownwardSweepEnd = clearanceHitMidToEnd.ImpactPoint;

            // Ensure a valid hit (i.e. didn't start penetrating)
            FHitResult lipDownwardSweepHit;
            if (SweepSphereSingle(lipDownwardSweepHit, Capsule, lipDownwardSweepStart, lipDownwardSweepEnd, lipSweepSphereRadius))
            {
               // Adjust the height of the forward sweep by the height difference between lip and surface
               float adjustedHeight = lipDownwardSweepHit.ImpactPoint.Z + capsuleHalfHeight;

               // Ensure it doesn't exceed projected max mantle height
               adjustedHeight = FMath::Min(adjustedHeight, TraceStart.Z);

               clearanceLocMid.Z = adjustedHeight;
               clearanceLocEnd.Z = adjustedHeight;

               // Sweep again from adjusted height
               clearedMidToEndSweep = !SweepMantleSingle(clearanceHitMidToEnd, Capsule, clearanceLocMid, clearanceLocEnd, kClearanceHeightScale, kClearanceRadiusScale);
            }
         }

         if (clearedMidToEndSweep)
         {
            // Upwards sweep from start to mid point. We use a shorter, narrower capsule to avoid grazing hits.
            FHitResult clearanceHitStartToMid;
            if (!SweepMantleSingle(clearanceHitStartToMid, Capsule, clearanceLocStart, clearanceLocMid, kClearanceHeightScale, kClearanceRadiusScale))
            {
               FOSEMantleQueryResult CandidateLocation = LocationData;
               CandidateLocation.FinalHitResult = CandidateHit;
               CandidateLocation.Result = EOSEMantleResult::Valid;
               TRY_VIS_LOG_CAPSULE(Capsule, LogOSEMantle, CandidateHit.Location, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FColor::Green);
               ValidMantleList.Add(CandidateLocation);
            }
         }
         else
         {               
            TRY_VIS_LOG_CAPSULE(Capsule, LogOSEMantle, clearanceHitMidToEnd.Location, kClearanceHeightScale, kClearanceRadiusScale, FColor::Orange);
         }
      }      
      else
      {
         TRY_VIS_LOG_CAPSULE(Capsule, LogOSEMantle, CandidateHit.Location, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FColor::Red);
      }
   }
}

// Utility method to get sweep params
const UWorld* UOSEMantleQuery::GetMantleSweepParams(
   FCollisionQueryParams& QueryParams,
   FCollisionResponseParams& ResponseParams,
   ECollisionChannel& CollisionChannel,
   const UCapsuleComponent* Capsule)
{
   const UWorld* World = (Capsule != nullptr) ? Capsule->GetWorld() : nullptr;
   if (World != nullptr)
   {
      // Query and response params
      QueryParams = FCollisionQueryParams(SCENE_QUERY_STAT(UOSEMantleQuery), false, Capsule->GetOwner());
      Capsule->InitSweepCollisionParams(QueryParams, ResponseParams);

      // Additional parameters
      QueryParams.bFindInitialOverlaps = false;
      QueryParams.bIgnoreTouches = true;
      QueryParams.bReturnFaceIndex = false;
      QueryParams.bReturnPhysicalMaterial = true;
#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING)
      QueryParams.bDebugQuery = true;
#endif

      // Collision channel
      CollisionChannel = Capsule->GetCollisionObjectType();
   }

   return World;
}

// Utility method for constructing a scaled capsule trace shape, with height/radius scaled by parameters
FCollisionShape UOSEMantleQuery::MakeCapsuleTraceShape(const UCapsuleComponent* capsule, const float heightScale /*= 1.0f*/, const float radiusScale /*= 1.0f*/)
{
   const float capsuleHalfHeight = capsule->GetScaledCapsuleHalfHeight() * heightScale;
   const float capsuleRadius = capsule->GetScaledCapsuleRadius() * radiusScale;
   const FVector capsuleExtents(capsuleRadius, capsuleRadius, capsuleHalfHeight);

   return FCollisionShape::MakeCapsule(capsuleExtents);
}

// Utility method for single capsule sweep
bool UOSEMantleQuery::SweepMantleSingle(
   FHitResult& HitResult,
   const UCapsuleComponent* Capsule,
   const FVector& TraceStart,
   const FVector& TraceEnd,
   const float HeightScale /* = 1.0f */,
   const float RadiusScale /* = 1.0f */)
{
   FCollisionShape TraceShape = MakeCapsuleTraceShape(Capsule, HeightScale, RadiusScale);
   FCollisionQueryParams QueryParams;
   FCollisionResponseParams ResponseParams;
   ECollisionChannel CollisionChannel;
   const UWorld* World = GetMantleSweepParams(QueryParams, ResponseParams, CollisionChannel,
      Capsule);

   if (World == nullptr)
   {
      return false;
   }
   TRY_VIS_LOG_CAPSULE_WIRE(Capsule->GetOwner(), LogOSEMantle, TraceStart, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FColor::Yellow)
   TRY_VIS_LOG_CAPSULE_WIRE(Capsule->GetOwner(), LogOSEMantle, TraceEnd, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FColor::Blue)

   if (!World->SweepSingleByChannel(
      HitResult, TraceStart, TraceEnd, Capsule->GetComponentQuat(),
      CollisionChannel, TraceShape, QueryParams, ResponseParams))
   {
      return false;
   }

   // Success
   return HitResult.IsValidBlockingHit();
}

// Utility method for multi capsule sweep
bool UOSEMantleQuery::SweepMantleMulti(
   TArray<FHitResult>& OutHits,
   const UCapsuleComponent* Capsule,
   const FVector& TraceStart,
   const FVector& TraceEnd,
   const float HeightScale /* = 1.0f */,
   const float RadiusScale /* = 1.0f */)
{
   FCollisionShape TraceShape = MakeCapsuleTraceShape(Capsule, HeightScale, RadiusScale);
   FCollisionQueryParams QueryParams;
   FCollisionResponseParams ResponseParams;
   ECollisionChannel CollisionChannel;
   const UWorld* World = GetMantleSweepParams(QueryParams, ResponseParams, CollisionChannel,
      Capsule);

   if (World == nullptr)
   {
      return false;
   }

   if (!World->SweepMultiByChannel(
      OutHits, TraceStart, TraceEnd, Capsule->GetComponentQuat(),
      CollisionChannel, TraceShape, QueryParams, ResponseParams))
   {
      return false;
   }

   // Success
   OutHits.RemoveAll([=](const FHitResult& hitResult) { return !hitResult.IsValidBlockingHit(); });
   return (OutHits.Num() > 0);
}

// Utility method for single sphere sweep
bool UOSEMantleQuery::SweepSphereSingle(FHitResult& hitResult, const UCapsuleComponent* capsule, const FVector& traceStart, const FVector& traceEnd, const float sphereRadius)
{
   // Only using the capsule to get the collision/trace params
   FCollisionShape traceShape = FCollisionShape::MakeSphere(sphereRadius);
   FCollisionQueryParams queryParams;
   FCollisionResponseParams responseParams;
   ECollisionChannel collisionChannel;
   const UWorld* world = GetMantleSweepParams(queryParams, responseParams, collisionChannel, capsule);

   if (world == nullptr)
   {
      return false;
   }

   if (!world->SweepSingleByChannel(hitResult, traceStart, traceEnd, capsule->GetComponentQuat(), collisionChannel, traceShape, queryParams, responseParams))
   {
      return false;
   }

   // Success
   return hitResult.IsValidBlockingHit();
}

// Utility method to validate final hit results. Uses similar logic to UCharacterMovementComponent::CanStepUp
// without the movement mode check, since we want to support mantle while falling.
bool UOSEMantleQuery::IsValidMantleResult(
   const FHitResult& hitResult,
   const UCapsuleComponent* Capsule)
{
   if (!hitResult.IsValidBlockingHit())
   {
      return false;
   }

   if (auto pawnOwner = Cast<APawn>(Capsule->GetOwner()))
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
bool UOSEMantleQuery::FilterValidMantleResults(
   TArray<FHitResult>& outHits,
   const class UCapsuleComponent* capsule)
{
   outHits.RemoveAll([capsule](const FHitResult& hitResult) { return !IsValidMantleResult(hitResult, capsule); });
   return (outHits.Num() > 0);
}

#undef TRY_VIS_LOG_CAPSULE
#undef TRY_VIS_LOG_CAPSULE_WIRE
