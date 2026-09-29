// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"

// tat
#include "Traversal/TATTeleportUtilities.h"

#include "TATToolWorldActorConstraint.generated.h"

class AActor;
struct FHitResult;

/// Represents a check that we can perform on a potential placement of a world actor
/// This will run after a trace, which already specifies trace profile and max range,
/// and is a check for any tool-specific properties that need to hold (enough space,
/// being placed on a surface, etc)
UCLASS(BlueprintType, Abstract)
class TAT_API UTATToolWorldActorConstraint : public UObject
{
   GENERATED_BODY()
public:
   /// Check if placing a world actor at a location satisfies its constraints
   UFUNCTION(BlueprintCallable)
   static bool CanPlaceWorldActor(TSubclassOf<UTATToolWorldActorConstraint> constraintClass, AActor* playerPawn, const FHitResult& hitResult);

protected:
   virtual bool _CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const;
};

/// Verify that we are placing an actor in a location where the placing player has enough room
/// to teleport to after the actor is placed there.
/// NOTE: This only means it holds for the player that places it, and other characters might not have enough room
UCLASS(BlueprintType)
class TAT_API UTATToolWorldActorConstraint_EnoughSpaceToTeleport : public UTATToolWorldActorConstraint
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly)
   FTATTeleportQuerySettings TeleportSettings;

protected:
   virtual bool _CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const override;
};

/// Requires that the world actor is placed on a surface: effectively this enforces 
/// that we are trying to place something in range and aren't looking off into the distance
UCLASS(BlueprintType)
class TAT_API UTATToolWorldActorConstraint_PlacedOnSurface : public UTATToolWorldActorConstraint
{
   GENERATED_BODY()
public:

protected:
   virtual bool _CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const override;
};

/// Similar to PlacedOnSurface above, but also does an overlap to check for Tool World Actors
UCLASS(BlueprintType, Blueprintable)
class TAT_API UTATToolWorldActorConstraint_PlacedOnSurfaceAvoidToolWorldActor : public UTATToolWorldActorConstraint
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly)
   float MinDistanceToOtherActors = 100.0f;
protected:
   virtual bool _CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const override;
};

/// A base class for any blueprint-derived constraints
UCLASS(BlueprintType, Abstract, Blueprintable)
class TAT_API UTATToolWorldActorConstraint_BlueprintBase : public UTATToolWorldActorConstraint
{
   GENERATED_BODY()

protected:
   UFUNCTION(BlueprintImplementableEvent)
   bool BP_CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const;

   virtual bool _CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const override;
};
