// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Traversal/WireWrap/WireWrapQuery.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WireWrapQuery)


FVector FWrapEndpoint::ToWorldPosition() const
{
   if (Type == EWrapEndpointType::Absolute)
   {
      return Offset;
   }
   else if (Type == EWrapEndpointType::ActorRelative && ensure(Actor))
   {
      return Actor->GetActorTransform().TransformPosition(Offset);
   }
   else
   {
      return FVector::ZeroVector;
   }
}

FWrapEndpoint FWrapEndpoint::FromWorldPosition(AActor* Actor, FVector WorldPosition)
{
   FWrapEndpoint Result;
   if (Actor)
   {
      Result.Actor = Actor;
      Result.Offset = Actor->GetActorTransform().InverseTransformPosition(WorldPosition);
      Result.Type = EWrapEndpointType::ActorRelative;
   }
   else
   {
      Result.Offset = WorldPosition;
      Result.Type = EWrapEndpointType::Absolute;
   }
   return Result;
}

FWrapEndpoint FWrapActorAndPosition::ToEndpoint() const
{
   return FWrapEndpoint::FromWorldPosition(Actor, WorldPosition);
}

namespace WireWrapQuery
{
   struct FWrapContext
   {
      FWrapContext(UWorld* InWorld, const FWrapSettings& InSettings, const TArray<AActor*>& InActorsToIgnore)
         : World(InWorld), QueryParams(SCENE_QUERY_STAT(WireWrapping)), Settings(InSettings)
      {
         QueryParams.bIgnoreTouches = true;
         QueryParams.AddIgnoredActors(InActorsToIgnore);
      }

      FWrapContext(const FWrapContext&) = delete;
      FWrapContext(FWrapContext&&) = delete;

      UWorld* World;
      FCollisionQueryParams QueryParams;
      const FWrapSettings& Settings;

      bool SphereTrace(FHitResult& Hit, const FVector& Start, const FVector& End, float RadiusOffset = 0) const
      {
         return World->SweepSingleByProfile(Hit, Start, End, FQuat::Identity, Settings.CollisionProfile, FCollisionShape::MakeSphere(Settings.TraceRadius+RadiusOffset), QueryParams);
      }

      bool LineTest(const FVector& Start, const FVector& End) const
      {
         return World->LineTraceTestByProfile(Start, End, Settings.CollisionProfile, QueryParams);
      }

      FVector GetOffsetPoint(const FHitResult& Hit) const
      {
         return Hit.Location + (Hit.Normal * Settings.OffsetDistance);
      }

      FWrapActorAndPosition GetOffsetPointWithActor(const FHitResult& Hit) const
      {
         return FWrapActorAndPosition(Hit.GetActor(), GetOffsetPoint(Hit));
      }
   };

   static void AddOrCollapse(const FWrapActorAndPosition& Point, TArray<FWrapActorAndPosition>& Results, const FWrapContext& Context)
   {
      if (Results.Num() == 0 || !Context.LineTest(Results.Last().WorldPosition, Point.WorldPosition))
      {
         Results.Add(Point);
      }
      else
      {
         Results.Last() = Point;
      }
   }

   static bool LowerBound(const FWrapContext& Context, const FVector& Target, FVector Start, FVector End, FWrapActorAndPosition& OutSegmentStart, FVector& OutSegmentEnd)
   {
      bool bFound = false;
      FWrapActorAndPosition Result;

      while (FVector::DistSquared(Start, End) > 1)
      {
         FVector Mid = FMath::Lerp(Start, End, 0.5f);
         FHitResult Hit;

         if (!Context.SphereTrace(Hit, Mid, Target))
         {
            Start = Mid;
            continue;
         }

         if (FVector::DistSquared(Hit.Location, Target) < FMath::Square(Context.Settings.MinSegmentLength))
         {
            Start = Mid;
            continue;
         }

         End = Mid;
         bFound = true;
         Result = Context.GetOffsetPointWithActor(Hit);
      }

      OutSegmentStart = Result;
      OutSegmentEnd = End;

      return bFound;
   }


   bool ComputeWireWrap(UObject* WorldContextObject, const FWrapSettings& Settings, const TArray<AActor*>& ActorsToIgnore,
      const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& PreviousSegmentEnd,
      TArray<FWrapActorAndPosition>& OutResults)
   {
      UWorld* World = WorldContextObject->GetWorld();
      FWrapContext Context(World, Settings, ActorsToIgnore);

      // Try the simple approach first
      {
         FHitResult Hit;

         if (!Context.SphereTrace(Hit, SegmentEnd, SegmentStart))
         {
            return false;
         }

         if (Hit.Distance < Settings.MinSegmentLength ||
            FVector::DistSquared(Hit.Location, SegmentStart) < FMath::Square(Settings.MinSegmentLength))
         {
            return false;
         }

         FWrapActorAndPosition CandidatePos = Context.GetOffsetPointWithActor(Hit);
         if (!Context.LineTest(CandidatePos.WorldPosition, SegmentStart))
         {
            OutResults.Add(CandidatePos);
            return true;
         }
      }

      // If the simple approach fails, try searching for a point in time where it would succeed and then repeat
      FVector Low = PreviousSegmentEnd;
      FVector High = SegmentEnd;
      FWrapActorAndPosition Target(nullptr, SegmentStart);
      while (true)
      {
         if (Context.LineTest(Low, Target.WorldPosition) || !Context.LineTest(High, Target.WorldPosition))
         {
            break;
         }

         if (!LowerBound(Context, Target.WorldPosition, Low, High, Target, Low))
         {
            break;
         }
         AddOrCollapse(Target, OutResults, Context);
      }

      return OutResults.Num() > 0;
   }

   bool CanUnwrap(UObject* WorldContextObject, const FWrapSettings& Settings,
      const TArray<AActor*>& ActorsToIgnore, const FVector& PreviousSegmentStart, const FVector& SegmentStart,
      const FVector& EndPosition)
   {
      UWorld* World = WorldContextObject->GetWorld();
      FWrapContext Context(World, Settings, ActorsToIgnore);

      FHitResult Hit;
      if(Context.SphereTrace(Hit, EndPosition, PreviousSegmentStart, Settings.OffsetDistance+Settings.UnwrapRadiusOffset))
      {
         return false;
      }

      FVector Point = FMath::ClosestPointOnSegment(SegmentStart, PreviousSegmentStart, EndPosition);
      return !Context.LineTest(SegmentStart, Point);
   }
}

