// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "WireWrapQuery.generated.h"

// Settings for Wire Wrapping queries
USTRUCT(BlueprintType)
struct OSECORE_API FWrapSettings
{
   GENERATED_BODY()

   // The collision profile to use when tracing for wrap collision
   UPROPERTY(EditDefaultsOnly)
   FName CollisionProfile;

   UPROPERTY(EditDefaultsOnly)
   float TraceRadius = 10;

   UPROPERTY(EditDefaultsOnly)
   float MinSegmentLength = 30;

   UPROPERTY(EditDefaultsOnly)
   float OffsetDistance = 5;

   UPROPERTY(EditDefaultsOnly)
   float UnwrapRadiusOffset = -2;
};

UENUM()
enum class EWrapEndpointType : uint8
{
   None,
   ActorRelative,
   Absolute
};


// The movable target of the wrap
USTRUCT(BlueprintType)
struct OSECORE_API FWrapEndpoint
{
   GENERATED_BODY()

   UPROPERTY()
   AActor* Actor = nullptr;

   UPROPERTY()
   FVector Offset = FVector(ForceInit);

   UPROPERTY()
   EWrapEndpointType Type = EWrapEndpointType::None;

   FVector ToWorldPosition() const;
   bool IsValid() const 
   { 
      return (Type == EWrapEndpointType::ActorRelative && ::IsValid(Actor)) || Type == EWrapEndpointType::Absolute;
   }

   static FWrapEndpoint FromWorldPosition(AActor* Actor, FVector WorldPosition);
   static FWrapEndpoint Empty()
   {
      FWrapEndpoint Point;
      return Point;
   }
};

// A world position along with an actor
struct OSECORE_API FWrapActorAndPosition
{
   FWrapActorAndPosition(AActor* Actor, FVector WorldPosition)
      : Actor(Actor), WorldPosition(WorldPosition)
   {}

   FWrapActorAndPosition()
      : Actor(nullptr), WorldPosition(ForceInitToZero)
   {}

   AActor* Actor;
   FVector WorldPosition;

   FWrapEndpoint ToEndpoint() const;
};

// CONSIDER: Is this small enough to to move into WireWrap proper?
namespace WireWrapQuery
{
   // Find new segment points to insert into a wire wrapping "simulation"
   bool ComputeWireWrap(
      UObject* WorldContextObject,
      const FWrapSettings& Settings,
      const TArray<AActor*>& ActorsToIgnore,
      const FVector& SegmentStart,
      const FVector& SegmentEnd,
      const FVector& PreviousSegmentEnd,
      TArray<FWrapActorAndPosition>& OutResults);

   bool CanUnwrap(
      UObject* WorldContextObject,
      const FWrapSettings& Settings,
      const TArray<AActor*>& ActorsToIgnore,
      const FVector& PreviousSegmentStart,
      const FVector& SegmentStart,
      const FVector& EndPosition);
}
