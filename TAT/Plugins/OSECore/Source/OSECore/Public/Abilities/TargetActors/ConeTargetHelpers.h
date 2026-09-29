// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/CollisionProfile.h"

class APawn;
class UWorld;
struct FCollisionQueryParams;
struct FCollisionObjectQueryParams;

namespace ConeTargetHelpers
{
   FVector FindClosestPointOnPawn(APawn* pawnActor, const FVector& traceStart, const FVector& traceEnd, FVector* closestPointOnTraceSegment = nullptr);

   struct FLineOfSightContext
   {
      UWorld* World;
      FCollisionQueryParams& Params;
      FCollisionProfileName TraceProfile;
      bool DrawDebug;

      FLineOfSightContext(UWorld* world, FCollisionQueryParams& params, FCollisionProfileName traceProfile, bool debug)
         : World(world), Params(params), TraceProfile(traceProfile), DrawDebug(debug)
      {}

      bool DoLineOfSightTrace(const FVector& traceStart, const FVector& traceEnd) const;
      bool HasLineOfSight(const FVector& traceStart, APawn* pawnActor) const;
   };

   // If multiple actors are in the cone, what should we use to pick the best one
   enum class EConeTraceRankCriteria : uint8
   {
      // Pick the one that has the shortest distance to the start of the trace
      ShortestDistanceToStart,

      // Pick the one that has the shortest distance to anywhere on the trace line
      ShortestDistanceToTraceLine,

      // Pick the one that has the smallest angle relative to our heading
      SmallestAngle
   };

   struct FConeTraceParams
   {
      const AActor* SourceActor = nullptr;
      FGameplayAbilityTargetingLocationInfo StartLocation;
      float MaxRange = 0;
      float HalfAngle = 0;
      FCollisionObjectQueryParams ObjectQueryParams;
      FCollisionProfileName LineOfSightProfile;
      bool DrawDebug;
      EConeTraceRankCriteria RankCriteria = EConeTraceRankCriteria::ShortestDistanceToStart;
   };

   struct FConeTraceResult
   {
      APawn* FoundActor;
      FVector TraceStart;
      FVector TraceEnd;
   };

   OSECORE_API FConeTraceResult DoConeTrace(const FConeTraceParams& params, TFunctionRef<bool(const AActor* actor)> filter);
}
