// (c) 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSETentacleState.generated.h"


//---------------------------------------------------------------------------------------
/// Simple structure to maintain and update the state of a tentacle
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSERENDERER_API FOSETentacleState
{
   GENERATED_BODY()

private:

   /// Optional component-space hit location; used to determine the world location
   /// to project towards when updating tentacle state.
   TOptional<FVector> _componentSpaceLocation;

public:

   /// Current hit result. Some information may be invalid for retracting tentacles;
   /// when retracting the hit result is no longer updated (except for the `TraceStart`
   /// member which is updated for convenience)
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FHitResult HitResult;

   /// Unique index when first generating the sphere rotation
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   int32 SphereIdx = INDEX_NONE;

   /// The initial sphere rotation
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FQuat SphereRotation = FQuat::Identity;

   /// Current length of the tentacle. Updated as needed when extended / retracted.
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float Length = 0;

   /// The amount of time this tentacle has left to "live" before retracting.
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float TimeRemaining = 0;

   /// Unique index corresponding to a specific visual data entry
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   int32 VisualDataIdx = INDEX_NONE;

public:

   /// Updates the current length value. Returns the updated distance from the target.
   float UpdateLength(float target, float speed, float deltaTime);

   /// Updates the component-space location from the hit result
   void UpdateComponentSpaceLocation();

   /// Returns the world-space location as derived from the hit result component-space location
   FVector GetWorldSpaceLocation() const;
};
