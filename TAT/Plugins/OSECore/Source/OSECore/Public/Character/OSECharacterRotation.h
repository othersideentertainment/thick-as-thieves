// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSECharacterRotation.generated.h"


USTRUCT(BlueprintType)
struct OSECORE_API FOSETurnInPlaceSettings
{
   GENERATED_BODY()

public:

   /// Defines the minimum value for the angle delta range.
   /// Uses the minimum countdown time at this angle delta.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0.1", UIMax = "179.9", ClampMin = "0.001", ClampMax = "179.999", Units = deg))
   float AngleDeltaMin = 80.0f;

   /// Defines the maximum value for the angle delta range.
   /// Uses the maximum countdown time when >= this angle delta.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0.1", UIMax = "179.9", ClampMin = "0.001", ClampMax = "179.999", Units = deg))
   float AngleDeltaMax = 140.0f;

   /// The time limit before activating turn-in-place when the angle delta is at the minimum value.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0", Units = s))
   float TimeLimitMin = 9.0f;

   /// The time limit before activating turn-in-place when the angle delta is at the maximum value.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0", ClampMin = "0", Units = s))
   float TimeLimitMax = 0.2f;
};


/// Run-time state for turn-in-place. Used to update turn-in-place and to
/// provide data useful for animation to take advantage of in ABPs.
USTRUCT(BlueprintType)
struct OSECORE_API FOSETurnInPlaceState
{
   GENERATED_BODY()

public:

   /// True when turn-in-place is currently active
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool IsActive = false;

   /// Total duration while turn-in-place was active
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Units = s))
   float ActiveDuration = 0;

   /// Accumulated time while approaching the limit to activate turn-in-place
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Units = s))
   float StartTimer = 0;

   /// The calculated rotation rate to use for turn-in-place.
   ///   (Note, this value is computed even when turn-in-place is not active)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Units = deg))
   float RotationRate = 0;

   /// The absolute value of the yaw rotation delta from the target.
   ///   (Note, this value is computed even when turn-in-place is not active)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Units = deg))
   float YawDeltaAbs = 0;

   /// The sign of the yaw rotation delta from the target.
   /// Useful for calculating direction.
   ///   (Note, this value is computed even when turn-in-place is not active)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float YawDeltaSign = 0;
};
